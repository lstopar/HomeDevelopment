void setup() {
  Serial.begin(9600);
  Serial.println("========================================");
  Serial.println("TEST");
  Serial.println("========================================");
  
  pinMode(0, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println(analogRead(0));
}
