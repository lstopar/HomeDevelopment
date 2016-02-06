const int pin = 3;
int val = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(pin, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  val += 1;
  val %= 255;
  analogWrite(pin, val);
  Serial.print("Set value to "); Serial.println(val);
  delay(10);
}
