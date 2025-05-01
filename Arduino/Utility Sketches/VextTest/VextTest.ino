/*
  Vext power source test

  Function:
  Turn Vext ON and OFF at 5 second intervals to allow voltage measurements at appropriate PCB test points

  1 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au

*/

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] Vext power source test");
  pinMode(Vext,OUTPUT);
}

void loop() {
  Serial.println("[loop] Turn Vext OFF");
  digitalWrite(Vext,HIGH);     // Turn off the external power supply
  delay(5000);
  Serial.println("[loop] Turn Vext ON");
  digitalWrite(Vext,LOW);     // Turn off the external power supply
  delay(5000);
}
