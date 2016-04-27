#include <Sync.h>
#include <RF24Network.h>
#include <RF24Network_config.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "protocol.h"
#include "arduino_home.h"

const uint16_t MY_ADDRESS = 02;
const int PIR_PIN = 4;
const int PIR_TV_PIN = 5;

const int LUM_PIN = 3;
//const int VOUT_PIN = 5;

const int N_VAL_IDS = 3;
const int VAL_IDS[N_VAL_IDS] = {
  PIR_PIN,
  LUM_PIN,
  PIR_TV_PIN
};

RF24 _radio(7,8);
RF24Network network(_radio);
TRf24Wrapper radio(MY_ADDRESS, _radio, network);

TAnalogPir pir(PIR_PIN, 1000, 500);
TDigitalPir tvPir(PIR_TV_PIN);

void processGet(const uint16_t&, const std::vector<char>&);
void processSet(const uint16_t&, const std::vector<TRadioValue>&);
void onPirEvent(const bool& motion);
void onTvPirEvent(const bool& motion);
//void onSwitchEvent(const bool& isOn);

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println("========================================");
  Serial.println("PIR");
  Serial.println("========================================");

  pinMode(PIR_PIN, INPUT);
  pinMode(LUM_PIN, INPUT);
  
  pir.init();
  tvPir.init();
 
  SPI.begin();

  radio.setProcessGet(processGet);
  radio.setProcessSet(processSet);
  radio.init();

  Serial.print("My address: ");
  Serial.println(MY_ADDRESS);

  // set callbacks
  pir.setCallback(onPirEvent);
  tvPir.setCallback(onTvPirEvent);
}

//====================================================
// GET/SET FUNCTIONS
//====================================================

bool getRadioVal(const char& valId, TRadioValue& rval) {
  rval.SetValId(valId);

  if (valId == PIR_PIN) {
    rval.SetVal(pir.isOn());
  }
  else if (valId == LUM_PIN) {
    rval.SetVal(analogRead(LUM_PIN) >> 2);
  }
  else if (valId == PIR_TV_PIN) {
    rval.SetVal(tvPir.isOn());
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId, HEX);
    return false;
  }

  return true;
}

void processGet(const uint16_t& callerAddr, const char& valId) {
  if (valId == VAL_ID_ALL) {
    std::vector<TRadioValue> rvals;

    for (int valN = 0; valN < N_VAL_IDS; valN++) {
      TRadioValue rval;
      getRadioVal(VAL_IDS[valN], rval);
      rvals.push_back(rval);
    }

    radio.push(callerAddr, rvals);
    
    Serial.println("Sent all values!");
  } else {
    TRadioValue radioVal;
    if (getRadioVal(valId, radioVal)) {
      radio.push(callerAddr, radioVal);
    }
  }
}

void processGet(const uint16_t& callerAddr, const std::vector<char>& valueIdV) {
  Serial.println("Processing GET request ...");
  for (int valN = 0; valN < valueIdV.size(); valN++) {
    const char& valId = valueIdV[valN];
    processGet(callerAddr, valId);
  }
  Serial.println("Processed!");
}

void processSet(const uint16_t& callerAddr, const std::vector<TRadioValue>& valV) {
  for (int valN = 0; valN < valV.size(); valN++) {
    const TRadioValue& rval = valV[valN];
    const char valId = rval.GetValId();
    Serial.print("Unknown valId for SET: ");  Serial.println(valId);
  }
}

//====================================================
// CALLBACK FUNCTIONS
//====================================================

void onPirEvent(const bool& motion) {
  processGet(ADDRESS_RPI, PIR_PIN);
}

void onTvPirEvent(const bool& motion) {
  processGet(ADDRESS_RPI, PIR_TV_PIN);
}

void loop(void) {
  radio.update();
  pir.update();
  tvPir.update();
}

