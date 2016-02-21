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
const int MODE_BLINK_RGB = 15;
const int MODE_CYCLE_HSV = 14;

const int N_VAL_IDS = 5;
const int VAL_IDS[N_VAL_IDS] = {
  LED_PIN,
  PIN_BLUE,
  PIN_RED,
  PIN_GREEN,
  MODE_BLINK_RGB
};

int pin3Val = 0;

RGBStrip rgb(PIN_RED, PIN_GREEN, PIN_BLUE);

RF24 radio(7,8);
RF24Network network(radio);

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);

char recPayload[PAYLOAD_LEN];
char sendPayload[PAYLOAD_LEN];

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
  rgb.reset();
 
  SPI.begin();

  TRadioProtocol::InitRadio(radio, network, MY_ADDRESS, RF24_PA_MAX);

  Serial.print("My address: ");
  Serial.println(MY_ADDRESS);
  Serial.print("All val ID: ");
  Serial.println(VAL_ID_ALL, HEX);
  Serial.print("Values per payload: ");
  Serial.println(VALS_PER_PAYLOAD);
  
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

void push(const uint16_t& to, const TRadioValue* valV, const int& len) {  
  TRadioProtocol::genPushPayload(valV, len, sendPayload);
  writeRadio(to, REQUEST_PUSH, sendPayload, PAYLOAD_LEN);
}

void push(const uint16_t& to, const TRadioValue& radioVal) {
  push(to, &radioVal, 1);
}

void getRadioVal(const char& valId, TRadioValue& rval) {
  rval.ValId = valId;

  if (valId == LED_PIN || valId == PIN_BLUE || valId == PIN_RED || valId == PIN_GREEN) {
    int val;
    
    switch (valId) {
    case LED_PIN:
      val = pin3Val;
      break;
    case PIN_BLUE:
      val = rgb.getBlue();
      break;
    case PIN_RED:
      val = rgb.getRed();
      break;
    case PIN_GREEN:
      val = rgb.getGreen();
      break;
    default:
      Serial.print("Unknown pin: ");
      Serial.println(valId);  
    }

    rval.Val = (int) (val / 2.55);
  } else if (valId == MODE_BLINK_RGB) {
    rval.Val = rgb.isBlinking() ? 1 : 0;
  }
  else if (valId == MODE_CYCLE_HSV) {
    rval.Val = rgb.isCyclingHsv() ? 1 : 0;
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
  else if (valId == LED_PIN || valId == PIN_BLUE || valId == PIN_RED || valId == PIN_GREEN) {
    TRadioValue rval;
    getRadioVal(valId, rval);
    push(callerAddr, rval);
  }
  else if (valId == MODE_BLINK_RGB || valId == MODE_CYCLE_HSV) {
    TRadioValue rval;
    getRadioVal(valId, rval);
    push(callerAddr, rval);
  } 
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId, HEX);
  }
}

void processSet(const uint16_t& callerAddr, const char& valId, const int& val) {
  if (valId == LED_PIN) {
    Serial.print("Setting LED ...");
    
    int transVal = (int) (double(val)*2.55);
    analogWrite(LED_PIN, transVal);
    pin3Val = transVal;
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_BLUE) {
    rgb.setBlue((int) (double(val)*2.55));
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_RED) {
    rgb.setRed((int) (double(val)*2.55));
    processGet(callerAddr, valId);
  }
  else if (valId == PIN_GREEN) {
    rgb.setGreen((int) (double(val)*2.55));
    processGet(callerAddr, valId);
  }
  else if (valId == MODE_BLINK_RGB) {
    if (val == 1) {
      rgb.blink();
    } else {
      rgb.reset();
    }
    
    processGet(callerAddr, valId);
  }
  else if (valId == MODE_CYCLE_HSV) {
    if (val == 1) {
      rgb.cycleHsv();
    } else {
      rgb.reset();
    }
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
      
      network.read(header, recPayload, PAYLOAD_LEN);

      char buff[VALS_PER_PAYLOAD];
      int nVals = TRadioProtocol::parseGetPayload(recPayload, buff);
      for (int valN = 0; valN < nVals; valN++) {
        const char& valId = buff[valN];
        processGet(fromAddr, valId);
      }
    } else if (header.type == REQUEST_SET) {
      network.read(header, recPayload, PAYLOAD_LEN);

      TRadioValue valV[VALS_PER_PAYLOAD];
      int vals = TRadioProtocol::parseSetPayload(recPayload, valV);
      for (int valN = 0; valN < vals; valN++) {
        const TRadioValue& val = valV[valN];
        processSet(fromAddr, val.ValId, val.Val);
      }
    } else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
    }
  }

  rgb.update();
}

