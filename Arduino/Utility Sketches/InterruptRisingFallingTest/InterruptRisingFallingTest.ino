/*
  Rising/Falling Interrupt test

  Function:
  Generate interrupts when the interrupt button is both pressed and released. The pin on which the interrupt is
  configured can be set by PCB identifier or manually.

  The function of this sketch differs from simply setting an interrupt to be triggered on a CHANGE, which would
  generate interrupts on both the RISING and FALLING edge of a pulse, in that it allows the time between the
  two, the RISING and FALLING (i.e. the duration of the button press), to be measured.

  1 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au
*/

#define buttonPin GPIO11

bool buttonPressFlag = false;
bool buttonReleaseFlag = false;

unsigned long pressTime, releaseTime;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] Interrupt test");

  PINMODE_INPUT_PULLUP( buttonPin );
  attachInterrupt( buttonPin, buttonPressed, FALLING );

}

void loop() {
  if ( buttonPressFlag ) {
    Serial.println("[loop] Button pressed...");
    pressTime = millis();
    buttonPressFlag = false;
  }

  if ( buttonReleaseFlag ) {
    Serial.println("[loop] Button released...");
    releaseTime = millis();
    Serial.print("[loop] Button hold time: ");
    Serial.print( releaseTime - pressTime );
    Serial.println(" milliseconds");
    buttonReleaseFlag = false;
  }
}

void buttonPressed() {
  buttonPressFlag = true;
  attachInterrupt( buttonPin, buttonReleased, RISING );
}

void buttonReleased() {
  buttonReleaseFlag = true;
  attachInterrupt( buttonPin, buttonPressed, FALLING );
}