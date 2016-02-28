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
const int PIR_TV_PIN = 4;
const int MODE_BLINK_RGB = 15;
const int MODE_CYCLE_HSV = 14;

const int N_VAL_IDS = 7;
const int VAL_IDS[N_VAL_IDS] = {
  LED_PIN,
  PIN_BLUE,
  PIN_RED,
  PIN_GREEN,
  PIR_TV_PIN,
  MODE_BLINK_RGB,
  MODE_CYCLE_HSV
};

int pin3Val = 0;

TRgbStrip rgb(PIN_RED, PIN_GREEN, PIN_BLUE);
TDigitalPir pir(PIR_TV_PIN);

RF24 radio(7,8);
RF24Network network(radio);

RF24NetworkHeader header;

char recPayload[PAYLOAD_LEN];
char sendPayload[PAYLOAD_LEN];
TRadioValue* setValV = new TRadioValue[VALS_PER_PAYLOAD];
char* getBuff = new char[VALS_PER_PAYLOAD];

//====================================================
// INITIALIZATION
//====================================================

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len);
void onPirEvent(const bool& motion);

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
  pir.init();
 
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

  pir.setCallback(onPirEvent);
}

//====================================================
// RADIO FUNCTIONS
//====================================================

void writeRadio(const uint16_t& recipient, const unsigned char& type, const char* buff, const int& len) {
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

//====================================================
// GET/SET FUNCTIONS
//====================================================

bool getRadioVal(const char& valId, TRadioValue& rval) {
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
  else if (valId == PIR_TV_PIN) {
    rval.Val = pir.isOn() ? 1 : 0;
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId, HEX);
    return false;
  }

  return true;
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
  } else {
    TRadioValue rval;
    if (getRadioVal(valId, rval)) {
      push(callerAddr, rval);
    }
  }
}

void processSet(const uint16_t& callerAddr, const char& valId, const int& val) {
  if (valId == LED_PIN) {    
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

    processGet(callerAddr, valId);
  }
  else {
    Serial.print("Unknown val ID: ");
    Serial.println(valId);
  }
}

//====================================================
// CALLBACK FUNCTIONS
//====================================================

void onPirEvent(const bool& motion) {
  processGet(ADDRESS_RPI, PIR_TV_PIN);
}

//====================================================
// MAIN LOOP
//====================================================

void loop(void) {
  network.update();

  while (network.available()) {
    network.peek(header);

    const uint16_t& fromAddr = header.from_node;
    if (header.type == REQUEST_CHILD_CONFIG) {
      network.read(header, NULL, 0);
      Serial.println("Received configuration message, ignoring ...");
    } else if (header.type == REQUEST_PING) {
      network.read(header, NULL, 0);
      Serial.println("Received ping, ignoring ...");
    } else if (header.type == REQUEST_GET) {
      network.read(header, recPayload, PAYLOAD_LEN);

      Serial.println("Received GET request ...");

      int nVals = TRadioProtocol::parseGetPayload(recPayload, getBuff);
      for (int valN = 0; valN < nVals; valN++) {
        const char& valId = getBuff[valN];
        processGet(fromAddr, valId);
      }
    } else if (header.type == REQUEST_SET) {
      network.read(header, recPayload, PAYLOAD_LEN);

      Serial.println("Received SET ...");

      const int vals = TRadioProtocol::parseSetPayload(recPayload, setValV);
      for (int valN = 0; valN < vals; valN++) {
        const TRadioValue& val = setValV[valN];
        processSet(fromAddr, val.ValId, val.Val);
      }
    } else {
      Serial.print("Unknown header type: ");
      Serial.println(header.type);
      network.read(header, NULL, 0);
    }
  }

  rgb.update();
  pir.update();
}
