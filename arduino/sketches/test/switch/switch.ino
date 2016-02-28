const int VOUT_PIN = 4;
const int READ_PIN = 3;
const int SWITCH_PIN = 6;

int currVal = LOW;

void setup() {
  Serial.begin(9600);

  Serial.println("========================================");
  Serial.println("SWITCH");
  Serial.println("========================================");
  
  pinMode(VOUT_PIN, OUTPUT);
  pinMode(READ_PIN, INPUT);
  pinMode(SWITCH_PIN, OUTPUT);

  digitalWrite(SWITCH_PIN, currVal);
}

void loop() {
  digitalWrite(VOUT_PIN, HIGH);
  delay(1);
  const int val = digitalRead(READ_PIN);
  digitalWrite(VOUT_PIN, LOW);
  Serial.println(val);

  if (val != currVal) {
    currVal = val;
    digitalWrite(SWITCH_PIN, currVal);
  }
}
