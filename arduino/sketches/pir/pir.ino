#include <Sync.h>
#include <RF24Network.h>
#include <RF24Network_config.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "protocol.h"

const uint16_t MY_ADDRESS = 02;
const int PIR_PIN = 4;
const int LUM_PIN = 3;
const int VOUT_PIN = 5;

const int N_VAL_IDS = 2;
const int VAL_IDS[N_VAL_IDS] = {
  PIR_PIN,
  LUM_PIN
};

const int PIR_THRESHOLD = 500;

RF24 radio(7,8);
RF24Network network(radio);

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);

bool currPirOn = false;
int lastPirVal = 0;
unsigned long lastOnTime = 0;

char payload[PAYLOAD_LEN];
//long int i = 0;

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println("========================================");
  Serial.println("PIR");
  Serial.println("========================================");

  pinMode(PIR_PIN, INPUT);
  pinMode(LUM_PIN, INPUT);
  
  pinMode(VOUT_PIN, OUTPUT);

  digitalWrite(VOUT_PIN, HIGH);
 
  SPI.begin();

  TRadioProtocol::InitRadio(radio, network, MY_ADDRESS, RF24_PA_MAX);

  Serial.print("My address: ");
  Serial.println(MY_ADDRESS);

  if (MY_ADDRESS != 00) {
    Serial.print("My parent is: ");
    Serial.println(network.parent());
    writeRadio(network.parent(), REQUEST_CHILD_CONFIG, NULL, 0);
  }
}

//====================================================
// RADIO FUNCTIONS
//====================================================

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len) {
//  Serial.print("Sending message to ");
//  Serial.println(recipient);
  RF24NetworkHeader header(recipient, type);
  network.write(header, buff, len);
}

void sendPirStatus(const uint16_t& recipient = ADDRESS_RPI) {
  TRadioProtocol::GenPushPayload(PIR_PIN, currPirOn ? 1 : 0, payload);
  writeRadio(recipient, REQUEST_PUSH, payload, PAYLOAD_LEN);
}

void readPIR() {
  int pirVal = analogRead(PIR_PIN);
  //Serial.println(pirVal);
  bool isOn = pirVal >= PIR_THRESHOLD;

//  Serial.println(pirVal);

  bool changed = false;

  if (isOn) {
    lastOnTime = millis();
    
    if (isOn != currPirOn) {
      Serial.println("Motion detected!");
      changed = true;
    }
    currPirOn = true; 
  } else if (isOn != currPirOn) {
    // the status has changed and the sensor is off
    if (millis() - lastOnTime > 1000) {
      currPirOn = false;
      Serial.println("Stopped detecting!");
      changed = true;
    }
  }

  if (changed) {
    sendPirStatus();
  }

  lastPirVal = pirVal;

//  if (i++ % 1000 == 0) {
////    Serial.println(lastPirVal);
//    TRadioProtocol::GenPushPayload(5, lastPirVal, payload);
//    writeRadio(ADDRESS_RPI, REQUEST_PUSH, payload, PAYLOAD_LEN);
//  }
}

void processGet(const uint16_t& callerAddr, const byte& valId) {
  if (valId == PIR_PIN) {
     sendPirStatus(callerAddr);
  } else if (valId == 5) {
    TRadioProtocol::GenPushPayload(5, lastPirVal, payload);
    writeRadio(callerAddr, REQUEST_PUSH, payload, PAYLOAD_LEN);
  } else if (valId == LUM_PIN) { 
    int lumVal = analogRead(LUM_PIN);
    Serial.print("Lum val: ");
    Serial.println(lumVal);
    TRadioProtocol::GenPushPayload(valId, int(lumVal / 10.23), payload);
    writeRadio(callerAddr, REQUEST_PUSH, payload, PAYLOAD_LEN);
  } else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void loop(void) {
  network.update();

  readPIR();

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
      
      network.read(header, payload, PAYLOAD_LEN);
      
      int valId;  TRadioProtocol::ParseGetPayload(payload, valId);
      
      processGet(fromAddr, valId);
    } else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
      network.read(header, NULL, 0);
    }
  }
}

