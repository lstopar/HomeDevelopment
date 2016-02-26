#include "protocol.h"

const int IN_PIN = 1;
const int VOUT_PIN = 2;
const int PWM_PIN = 9;

TManualDimmer dimmer(VOUT_PIN, IN_PIN, PWM_PIN, 20, 1000);

void setup() {
  Serial.begin(9600);

  Serial.println("========================================");
  Serial.println("POTENTIOMETER");
  Serial.println("========================================");

  dimmer.init();
  
  delay(1000);
}

void loop() {
  dimmer.update();
  Serial.println(dimmer.getVal());
  delay(100);
}
