#include "protocol.h"

const int PIR_PIN = 2;

TDigitalPir pir(PIR_PIN);

void onPirEvent(const bool& motionDetected);

void setup() {
  Serial.begin(9600);
  Serial.println("========================================");
  Serial.println("TESTING DIGITAL PIR");
  Serial.println("========================================");
  
  // put your setup code here, to run once:
  pir.init();
  pir.setCallback(onPirEvent);
}

void onPirEvent(const bool& motionDetected) {
  if (motionDetected) {
    Serial.println("ACTIVE==============================!");
  }
  else {
    Serial.println("inactive!");
  }
}

void loop() {
  pir.update();
}
