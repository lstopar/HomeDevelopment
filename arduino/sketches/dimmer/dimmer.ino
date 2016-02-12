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

int pin3Val = 0;

RF24 radio(7,8);
RF24Network network(radio);

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println("========================================");
  Serial.println("DIMMER");
  Serial.println("========================================");
 
  SPI.begin();

  TRadioProtocol::InitRadio(radio, network, MY_ADDRESS);
  
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
  if (valId == LED_PIN) {
     int val = (int) (pin3Val / 2.55);
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
  } else {
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

