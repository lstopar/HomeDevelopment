#include <Sync.h>
#include <RF24Network.h>
#include <RF24Network_config.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include "protocol.h"
#include "arduino_home.h"

const uint16_t MY_ADDRESS = 01;


const int LED_PIN = 3;
const int PIN_BLUE = 5;
const int PIN_RED = 6;
const int PIN_GREEN = 9;
const int MODE_BLINK_RGB = 15;
const int MODE_CYCLE_HSV = 14;

const int N_VAL_IDS = 6;
const int VAL_IDS[N_VAL_IDS] = {
  LED_PIN,
  PIN_BLUE,
  PIN_RED,
  PIN_GREEN,
  MODE_BLINK_RGB,
  MODE_CYCLE_HSV
};

int pin3Val = 0;

TRgbStrip rgb(PIN_RED, PIN_GREEN, PIN_BLUE);

RF24 _radio(7,8);
RF24Network network(_radio);
TRf24Wrapper radio(MY_ADDRESS, _radio, network);

//====================================================
// INITIALIZATION
//====================================================

void processGet(const uint16_t&, const std::vector<char>&);
void processSet(const uint16_t&, const std::vector<TRadioValue>&);
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
 
  SPI.begin();

  radio.setProcessGet(processGet);
  radio.setProcessSet(processSet);
  radio.init();
  
  Serial.print("My address: ");
  Serial.println(MY_ADDRESS);
  Serial.print("All val ID: ");
  Serial.println(VAL_ID_ALL, HEX);
  Serial.print("Values per payload: ");
  Serial.println(VALS_PER_PAYLOAD);
}

//====================================================
// GET/SET FUNCTIONS
//====================================================

bool getRadioVal(const char& valId, TRadioValue& rval) {
  rval.SetValId(valId);

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

    rval.SetVal(val);
  } else if (valId == MODE_BLINK_RGB) {
    rval.SetVal(rgb.isBlinking());
  }
  else if (valId == MODE_CYCLE_HSV) {
    rval.SetVal(rgb.isCyclingHsv());
  }
  else {
    Serial.print("Unknown val ID: "); Serial.println(valId, HEX);
    return false;
  }

  return true;
}

void internalGet(const char& valId, std::vector<TRadioValue>& valV) {
  if (valId == VAL_ID_ALL) {
    for (int valN = 0; valN < N_VAL_IDS; valN++) {
      TRadioValue rval;
      getRadioVal(VAL_IDS[valN], rval);
      valV.push_back(rval);
    }
    
  } else {
    TRadioValue rval;
    if (getRadioVal(valId, rval)) {
      valV.push_back(rval);
    }
  }
}


//====================================================
// CALLBACK FUNCTIONS
//====================================================


void processGet(const uint16_t& callerAddr, const std::vector<char>& valueIdV) {
  Serial.println("Processing GET request ...");

  std::vector<TRadioValue> valV;
  
  for (int valN = 0; valN < valueIdV.size(); valN++) {
    const char& valId = valueIdV[valN];
    internalGet(valId, valV);
  }

  if (!valV.empty()) {
    radio.push(callerAddr, valV);
  }
  
  Serial.println("Processed!");
}

void processSet(const uint16_t& callerAddr, const std::vector<TRadioValue>& valV) {
  std::vector<TRadioValue> outV;
  
  for (int valN = 0; valN < valV.size(); valN++) {
    const TRadioValue& rval = valV[valN];
    const char valId = rval.GetValId();
  
    if (valId == LED_PIN) {    
      int transVal = rval.GetValInt();
      analogWrite(LED_PIN, transVal);
      pin3Val = transVal;
      internalGet(valId, outV);
    }
    else if (valId == PIN_BLUE) {
      rgb.setBlue(rval.GetValInt());
      internalGet(valId, outV);
    }
    else if (valId == PIN_RED) {
      rgb.setRed(rval.GetValInt());
      internalGet(valId, outV);
    }
    else if (valId == PIN_GREEN) {
      rgb.setGreen(rval.GetValInt());
      internalGet(valId, outV);
    }
    else if (valId == MODE_BLINK_RGB) {
      if (rval.GetValBool()) {
        rgb.blink();
      } else {
        rgb.reset();
      }
      
      internalGet(valId, outV);
    }
    else if (valId == MODE_CYCLE_HSV) {
      if (rval.GetValBool()) {
        rgb.cycleHsv();
      } else {
        rgb.reset();
      }
  
      internalGet(valId, outV);
    }
    else {
      Serial.print("Unknown val ID: ");
      Serial.println(valId);
    }
  }

  if (!outV.empty()) {
    radio.push(callerAddr, outV);
  }
}

//====================================================
// MAIN LOOP
//====================================================

void loop(void) {
  radio.update();
  rgb.update();
//  pir.update();
}

