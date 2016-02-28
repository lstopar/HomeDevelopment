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

const int LIGHT_SWITCH_PIN = 6;
const int LIGHT_VOUT_PIN = 9;
const int LIGHT_READ_PIN = 2;

const int N_VAL_IDS = 3;
const int VAL_IDS[N_VAL_IDS] = {
  PIR_PIN,
  LUM_PIN,
  LIGHT_SWITCH_PIN
};

const int PIR_THRESHOLD = 500;

RF24 radio(7,8);
RF24Network network(radio);

TManualSwitch lightSwitch(LIGHT_VOUT_PIN, LIGHT_READ_PIN, LIGHT_SWITCH_PIN);

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);

bool currPirOn = false;
int lastPirVal = 0;
unsigned long lastOnTime = 0;

char recPayload[PAYLOAD_LEN];
char sendPayload[PAYLOAD_LEN];

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

  lightSwitch.init();
 
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
  Serial.println("Sending message ...");
  RF24NetworkHeader header(recipient, type);
  network.write(header, buff, len);
}

void push(const uint16_t& to, const TRadioValue* valV, const int& len) {  
  TRadioProtocol::genPushPayload(valV, len, sendPayload);
  writeRadio(to, REQUEST_PUSH, sendPayload, PAYLOAD_LEN);
}

void push(const uint16_t& to, const TRadioValue& radioVal) {
  push(to, &radioVal, 1);
}

void sendPirStatus(const uint16_t& recipient = ADDRESS_RPI) {
  TRadioValue rval;
  getRadioVal(PIR_PIN, rval);
  push(ADDRESS_RPI, rval);
}

void readPIR() {
  int pirVal = analogRead(PIR_PIN);
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
}

void getRadioVal(const char& valId, TRadioValue& rval) {
  rval.ValId = valId;

  if (valId == PIR_PIN) {
    rval.Val = currPirOn ? 1 : 0;
  }
  else if (valId == LUM_PIN) {
    rval.Val = analogRead(LUM_PIN);
  }
  else if (valId == LIGHT_SWITCH_PIN) {
    rval.Val = lightSwitch.isOn() ? 1 : 0;
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId, HEX);
  }
}

void processGet(const uint16_t& callerAddr, const char& valId) {
  if (valId == VAL_ID_ALL) {
    TRadioValue rvals[VALS_PER_PAYLOAD];
    
    int nsent = 0;
    int valN;
    while (nsent < N_VAL_IDS) {
      valN = 0;
      while (valN < VALS_PER_PAYLOAD && nsent + valN < N_VAL_IDS) {
        getRadioVal(VAL_IDS[nsent + valN], rvals[valN]);
        valN++;
      }

      push(callerAddr, rvals, valN);
      nsent += valN;
    }

    Serial.println("Sent all values!");
  }
  else if (valId == PIR_PIN) {
     sendPirStatus(callerAddr);
  } 
  else if (valId == LUM_PIN) {
    TRadioValue radioVal;
    getRadioVal(valId, radioVal);
    push(callerAddr, radioVal);
  }
  else if (valId == LIGHT_SWITCH_PIN) {
    TRadioValue radioVal;
    getRadioVal(valId, radioVal);
    push(callerAddr, radioVal);
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId);
  }
}

void processSet(const uint16_t& callerAddr, const char& valId, const int& val) {
  if (valId == LIGHT_SWITCH_PIN) {
    if (val == 1) {
      lightSwitch.on();
    } else {
      lightSwitch.off();
    }
    processGet(callerAddr, valId);
  } else {
    Serial.print("Unknown valId for SET: ");  Serial.println(valId);
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
    } 
    else if (header.type == REQUEST_PING) {
      network.read(header, NULL, 0);
      Serial.println("Received ping, ignoring ...");
    } 
    else if (header.type == REQUEST_GET) {
      Serial.println("Received GET request ...");
      
      network.read(header, recPayload, PAYLOAD_LEN);

      char buff[VALS_PER_PAYLOAD];
      int nVals = TRadioProtocol::parseGetPayload(recPayload, buff);
      for (int valN = 0; valN < nVals; valN++) {
        const char& valId = buff[valN];
        processGet(fromAddr, valId);
      }
    }
    else if (header.type == REQUEST_SET) {
      Serial.println("Received SET request ...");
      network.read(header, recPayload, PAYLOAD_LEN);

      TRadioValue receiveBuff[VALS_PER_PAYLOAD];
      int vals = TRadioProtocol::parseSetPayload(recPayload, receiveBuff);
      for (int valN = 0; valN < vals; valN++) {
        const TRadioValue& val = receiveBuff[valN];
        processSet(fromAddr, val.ValId, val.Val);
      }
    }
    else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
      network.read(header, NULL, 0);
    }
  }

  const bool isLightOn = lightSwitch.isOn();
  lightSwitch.update();

  if (lightSwitch.isOn() != isLightOn) {
    processGet(ADDRESS_RPI, LIGHT_SWITCH_PIN);
  }
}
