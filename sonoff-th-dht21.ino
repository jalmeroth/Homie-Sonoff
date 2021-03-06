#include <Homie.h>
#include "DHT.h"

#define PIN_BUTTON 0
#define PIN_RELAY 12
#define PIN_LED 13
#define DHT_PIN 14
#define DHT_TYP DHT21
#define INTERVAL 30

DHT dht(DHT_PIN, DHT_TYP);

HomieNode switchNode("switch", "switch");
HomieNode temperatureNode("temperature", "temperature");

#define FW_NAME "sonoff-th-dht21"
#define FW_VERSION "1.0.2"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

int relayState = LOW; 
int lastButtonState = LOW;           // the previous reading from the input pin
unsigned long lastSent = 0;
unsigned long debounceTime = 0;      // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

bool switchOnHandler(String value) {
  if (value == "true") {
    digitalWrite(PIN_RELAY, HIGH);
    Homie.setNodeProperty(switchNode, "on", "true", true);
    relayState = HIGH;
    Serial.println("Switch is on");
  } else if (value == "false") {
    digitalWrite(PIN_RELAY, LOW);
    Homie.setNodeProperty(switchNode, "on", "false", true);
    relayState = LOW;
    Serial.println("Switch is off");
  } else {
    return false;
  }
  return true;
}

void setupHandler() {
  // publish current relayState when online
  Homie.setNodeProperty(switchNode, "on", (relayState == HIGH) ? "true" : "false", true);
}

void loopHandler() {
  if (millis() >= (lastSent + (INTERVAL * 1000UL)) || lastSent == 0) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(false);
    
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    if (!Homie.setNodeProperty(temperatureNode, "degrees", String(temperature), true)) {
      Serial.println("Temperature sending failed");
    }

    Serial.print("Humidity: "); Serial.print(humidity); Serial.println("%");
    if (!Homie.setNodeProperty(temperatureNode, "humidity", String(humidity), true)) {
      Serial.println("Humidity sending failed");
    }

    lastSent = millis();
  }
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, relayState);
  Homie.setFirmware(FW_NAME, FW_VERSION);
  Homie.setLedPin(PIN_LED, LOW);
  Homie.setResetTrigger(PIN_BUTTON, LOW, 5000);
  Homie.registerNode(switchNode);
  Homie.registerNode(temperatureNode);
  switchNode.subscribe("on", switchOnHandler);
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonState) {
    // start timer for the first time
    if (debounceTime == 0) {
      debounceTime = millis();
    }

    if(reading == LOW) {
      Serial.println("Button push");
      // restart timer
      debounceTime = millis();
    } else {
      Serial.println("Button release");
      // let a unicorn pass 
      if ((millis() - debounceTime) > debounceDelay) {
        // invert relayState
        relayState = !relayState;
        if (Homie.isReadyToOperate()) {
          // normal mode and network connection up
          switchOnHandler((relayState == HIGH) ? "true" : "false");
        } else {
          // not in normal mode or network connection down
          digitalWrite(PIN_RELAY, relayState);
        }
      }
    }
  }
  lastButtonState = reading;

  Homie.loop();
}
