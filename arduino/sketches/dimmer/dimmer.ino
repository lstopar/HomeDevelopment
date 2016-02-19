#include <Sync.h>
#include <RF24Network.h>
#include <RF24Network_config.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "protocol.h"

const uint16_t MY_ADDRESS = 01;

const int LED_PIN = 3;
const int PIN_BLUE = 5;
const int PIN_RED = 6;
const int PIN_GREEN = 9;
const int MODE_BLINK_RGB = 1;

const int RGB_PINS[3] = { PIN_BLUE, PIN_RED, PIN_GREEN };

int rgbVals[3] = { 0, 0, 0 };
int pin3Val = 0;

bool blinkRgbStrip = false;
int currIncreaseClrN = 0;

RF24 radio(7,8);
RF24Network network(radio);

int iterN = 0;

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);
void resetRgb();

void setup() {
  bitSet(TCCR1B, WGM12);  // put PWM timer 1 into fast mode
  
  Serial.begin(9600);
  printf_begin();
  Serial.println("========================================");
  Serial.println("DIMMER");
  Serial.println("========================================");

  pinMode(LED_PIN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);

  analogWrite(LED_PIN, 0);
  resetRgb();
 
  SPI.begin();

  TRadioProtocol::InitRadio(radio, network, MY_ADDRESS, RF24_PA_MAX);

  Serial.print("My address: ");
  Serial.println(MY_ADDRESS);
  
  if (MY_ADDRESS != 00) {
    writeRadio(network.parent(), REQUEST_CHILD_CONFIG, NULL, 0);
  }
}

//====================================================
// RADIO FUNCTIONS
//====================================================

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len) {
  Serial.println("Sending message ...");
  RF24NetworkHeader header(recipient, type);
  network.write(header, buff, len);
}

void processGet(const uint16_t& callerAddr, const byte& valId) {
  if (valId == LED_PIN || valId == PIN_BLUE || valId == PIN_RED || valId == PIN_GREEN) {
    int val;
    switch (valId) {
    case LED_PIN:
      val = pin3Val;
      break;
    case PIN_BLUE:
      val = rgbVals[0];
      break;
    case PIN_RED:
      val = rgbVals[1];
      break;
    case PIN_GREEN:
      val = rgbVals[2];
      break;
    default:
      Serial.print("Unknown pin: ");
      Serial.println(valId);  
    }

    val = (int) (val / 2.55);

    Serial.print("Sending valId: ");
    Serial.print(valId);
    Serial.print(", value: ");
    Serial.println(val);

    char payload[PAYLOAD_LEN];
    TRadioProtocol::GenPushPayload(valId, val, payload);

    writeRadio(callerAddr, REQUEST_PUSH, payload, PAYLOAD_LEN);
  }
  else if (valId == MODE_BLINK_RGB) {
    char payload[PAYLOAD_LEN];
    TRadioProtocol::GenPushPayload(valId, blinkRgbStrip ? 1 : 0, payload);
    writeRadio(callerAddr, REQUEST_PUSH, payload, PAYLOAD_LEN);
  } 
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void processSet(const uint16_t& callerAddr, const byte& valId, const int& val) {
  if (valId == LED_PIN) {
    Serial.print("Setting LED ...");
    
    int transVal = (int) (double(val)*2.55);
    analogWrite(LED_PIN, transVal);
    pin3Val = transVal;
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_BLUE) {
     rgbVals[0] = (int) (double(val)*2.55);
     analogWrite(RGB_PINS[0], rgbVals[0]);
     processGet(callerAddr, valId);
  }
  else if (valId == PIN_RED) {
    rgbVals[1] = (int) (double(val)*2.55);
    analogWrite(RGB_PINS[1], rgbVals[1]);
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_GREEN) {
    rgbVals[2] = (int) (double(val)*2.55);
    analogWrite(RGB_PINS[2], rgbVals[2]);
    processGet(callerAddr, valId);
  }
  else if (valId == MODE_BLINK_RGB) {
    blinkRgbStrip = val == 1;
    currIncreaseClrN = 0;
    processGet(callerAddr, valId);
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void resetRgb() {
  for (int i = 0; i < 3; i++) {
    rgbVals[i] = 0;
    analogWrite(RGB_PINS[i], rgbVals[i]);
  }
}

void loop(void) {
  network.update();

  while (network.available()) {
    Serial.println("Reading message ...");
    
    RF24NetworkHeader header;
    network.peek(header);

    Serial.println("Received message ...");

    const uint16_t& fromAddr = header.from_node;
    if (header.type == REQUEST_CHILD_CONFIG) {
      network.read(header, NULL, 0);
      Serial.println("Received configuration message, ignoring ...");
    } else if (header.type == REQUEST_PING) {
      network.read(header, NULL, 0);
      Serial.println("Received ping, ignoring ...");
    } else if (header.type == REQUEST_GET) {
      Serial.println("Received GET request ...");
      
      char payload[PAYLOAD_LEN];
      network.read(header, payload, PAYLOAD_LEN);
      
      int valId;  TRadioProtocol::ParseGetPayload(payload, valId);
      
      processGet(fromAddr, valId);
    } else if (header.type == REQUEST_SET) {
      char payload[PAYLOAD_LEN];
      network.read(header, payload, PAYLOAD_LEN);

      int valId, val;
      TRadioProtocol::ParseSetPayload(payload, valId, val);
      
      processSet(fromAddr, valId, val);
    } else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
    }
  }

  if (blinkRgbStrip && iterN % 500 == 0) {
    rgbVals[currIncreaseClrN]++;

    if (rgbVals[currIncreaseClrN] == 255) {
      resetRgb();
      currIncreaseClrN++;
      currIncreaseClrN = currIncreaseClrN % 3;
    }
    
    analogWrite(RGB_PINS[currIncreaseClrN], rgbVals[currIncreaseClrN]);
  }

  iterN++;
}

