
const int PIN_GREEN = 9;
const int PIN_RED = 10;
const int PIN_BLUE = 11;

int pins[3] = { PIN_GREEN, PIN_RED, PIN_BLUE };

int pinVal = 0;
int pinN;
int i = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("========================================");
  Serial.println("RGB test");
  Serial.println("========================================");
  
  for (int i = 0; i < 3; i++) {
    pinMode(pins[i], OUTPUT);
    analogWrite(pins[i], LOW);
  }

  pinN = 0;

  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  //i % (2*256);
  int dir = 1;//i % (2*256) < 256 ? 1 : -1;

  if (i % 2 == 0) {
    Serial.print("Setting pin ");
    Serial.print(pins[pinN]);
    Serial.print(" to ");
    Serial.println(pinVal);
  }

  analogWrite(pins[pinN], pinVal);
  pinVal += dir;
  if (pinVal == 255) {
    for (int i = 0; i < 3; i++) {
      analogWrite(pins[i], LOW);
    }

    delay(2000);
    
    pinVal = 0;
    pinN += 1;
    pinN %= 3;

    Serial.print("Changed pin: ");
    Serial.println(pinN);
  }

  i++;
  delay(50);
}
