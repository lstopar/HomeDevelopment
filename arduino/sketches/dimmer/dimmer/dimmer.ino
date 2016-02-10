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

const byte NODE_ID = 1;

const byte COMMAND_GET = 0x00;
const byte COMMAND_SET = 0x01;
const byte COMMAND_PUSH = 0x02;
const byte COMMAND_PING = 0xFF;

const int LED_PIN = 3;

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(7,8);

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

const int PAYLOAD_LEN = 8;
const int CHANNEL = 0x4C;

byte received[PAYLOAD_LEN]; 

// Role management: Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles
role_e role = role_pong_back;                                              // The role of the current running sketch

// A single byte to keep track of the data being sent back and forth
int currPin3Val = 0;

void setup(){

  Serial.begin(9600);
  printf_begin();
  Serial.print(F("\n\rRF24/examples/pingpair_ack/\n\rROLE: "));
  Serial.println(role_friendly_name[role]);
  Serial.println(F("*** PRESS 'T' to begin transmitting to the other node"));

  Serial.println("Iniitalizing pins ...");
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Iniitalizing radio ...");
  // Setup and configure rf radio
  radio.begin();
  radio.setAutoAck(true);                    // Ensure autoACK is enabled
  radio.setRetries(15,15);                 // Smallest time between retries, max no. of retries
  radio.setChannel(CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(PAYLOAD_LEN);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.setCRCLength(RF24_CRC_8);
  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
  radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();                 // Start listening
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
     int val = (int) (currPin3Val / 2.55);
     Serial.print("Sending value: ");
     Serial.println(val);
     
     const byte payload[PAYLOAD_LEN] = {
      NODE_ID,
      COMMAND_PUSH,
      valId,
      0x00, // TODO
      0x00, // TODO
      byte(val >> 8),
      byte(val),
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
    currPin3Val = transVal;
    processGet(valId);
  } else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void loop(void) {
  // Pong back role.  Receive each packet, dump it out, and send it back
  
  byte pipeNo;
  while(radio.available(&pipeNo)) {
    radio.read(received, PAYLOAD_LEN);

    const byte nodeId = received[0];
    const byte command = received[1];

    Serial.print("Got msg on pipe: ");
    Serial.println(pipeNo);

    if (received[0] != NODE_ID) { continue; }

    Serial.println("Msg is for me!");

    if (command == COMMAND_PING) {
      Serial.println("Received ping, ignoring ...");
    } else if (command == COMMAND_GET) {
      const byte valId = received[2];
      processGet(valId);
    } else if (command == COMMAND_SET) {
      const byte valId = received[2];
      const int val = 0x00 +   // TODO
                      0x00 +   // TODO
                      (int(received[5]) << 8) + 
                      int(received[6]);
      processSet(valId, val);
    } else {
      Serial.print("Unknown command: ");
      Serial.println(command);
    }
  }
}

