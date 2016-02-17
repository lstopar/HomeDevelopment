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

int pin3Val = 0;
int pinBlueVal = 0;
int pinRedVal = 0;
int pinGreenVal = 0;

RF24 radio(7,8);
RF24Network network(radio);

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);

void setup() {
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
  analogWrite(PIN_BLUE, 0);
  analogWrite(PIN_RED, 0);
  analogWrite(PIN_GREEN, 0);
 
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
      val = pinBlueVal;
      break;
    case PIN_RED:
      val = pinRedVal;
      break;
    case PIN_GREEN:
      val = pinGreenVal;
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
  } else {
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
     pinBlueVal = (int) (double(val)*2.55);
     analogWrite(PIN_BLUE, pinBlueVal);
     processGet(callerAddr, valId);
  }
  else if (valId == PIN_RED) {
    pinRedVal = (int) (double(val)*2.55);
    analogWrite(PIN_RED, pinRedVal);
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_GREEN) {
    pinGreenVal = (int) (double(val)*2.55);
    analogWrite(PIN_GREEN, pinGreenVal);
    processGet(callerAddr, valId);
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
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
}

