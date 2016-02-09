#include <Sync.h>
#include <RF24Network.h>
#include <RF24Network_config.h>

/*
  // March 2014 - TMRh20 - Updated along with High Speed RF24 Library fork
  // Parts derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads 
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionally as well.
 * This allows very fast call-response communication, with the responding radio never having to 
 * switch out of Primary Receiver mode to send back a payload, but having the option to if wanting
 * to initiate communication instead of respond to a commmunication.
 */
 


#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

const int CHANNEL = 0x4C;
const uint16_t MY_ADDRESS = 01;

const int PAYLOAD_LEN = 8;
const unsigned char COMMAND_GET = 65;
const unsigned char COMMAND_SET = 66;
const unsigned char COMMAND_PUSH = 67;
const unsigned char COMMAND_PING = 't';

const int LED_PIN = 3;

RF24 radio(7,8);
RF24Network network(radio);

int pin3Val = 0;

void setup(){

  Serial.begin(9600);
  printf_begin();

  Serial.println("Iniitalizing pins ...");
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Iniitalizing radio ...");
  // Setup and configure rf radio
  radio.begin();
  radio.setDataRate(RF24_2MBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging

  Serial.println("Iniitalizing network ...");
  network.begin(CHANNEL, MY_ADDRESS);
  Serial.println("Initialization complete!");
}

//====================================================
// RADIO FUNCTIONS
//====================================================

void writeRadio(const byte* buff, const int& len) {
  Serial.println("Sending message ...");
  radio.stopListening();
  radio.write(buff, len);
  radio.startListening();
}

void processGet(const byte& valId) {
  if (valId == LED_PIN) {
     int val = (int) (pin3Val / 2.55);
     Serial.print("Sending value: ");
     Serial.println(val);
     
     const byte payload[PAYLOAD_LEN] = {
      valId,
      byte(val >> 24),
      byte(val >> 16),
      byte(val >> 8),
      byte(val),
      0xFF,
      0xFF,
      0xFF
     };

     writeRadio(payload, PAYLOAD_LEN);
  } else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void processSet(const byte& valId, const int& val) {
  if (valId == LED_PIN) {
    Serial.print("Setting LED ...");
    
    int transVal = (int) (double(val)*2.55);
    analogWrite(LED_PIN, transVal);
    pin3Val = transVal;
    processGet(valId);
  } else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void loop(void) {
  network.update();

  while (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);

    Serial.println("Received message ...");

    if (header.type == 't') {
      network.read(header, NULL, 0);
      Serial.println("Received ping, ignoring ...");
    } else if (header.type == COMMAND_GET) {
      Serial.println("Received GET request ...");
      byte payload[PAYLOAD_LEN];
      network.read(header, payload, PAYLOAD_LEN);
      processGet(payload[0]);
    } else if (header.type == COMMAND_SET) {
      byte payload[PAYLOAD_LEN];
      network.read(header, payload, PAYLOAD_LEN);

      int val = int(payload[1] << 24) |
                int(payload[2] << 16) |
                int(payload[3] << 8) |
                int(payload[4]);
      
      processSet(payload[0], val);
    } else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
    }
  }
}

