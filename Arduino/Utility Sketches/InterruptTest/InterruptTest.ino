/*
  Interrupt test

  Function:
  Generate an interrupt when the interrupt button is pressed. The pin on which the interrupt is
  configured can be set by PCB identifier or manually. Interrupts can be set to trigger on the
  RISING or FALLING edge, or simply on a CHANGE (both RISING and FALLING).

  1 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au
*/

#define interruptPin GPIO11   // Interrupt button on 10068-BHCP PCB

bool interruptFlag = false;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] Interrupt test");

  PINMODE_INPUT_PULLUP(interruptPin);
  attachInterrupt( interruptPin, isr, FALLING );
}

void loop() {
  if ( interruptFlag ) {
    Serial.println("[loop] Falling interrupt detected"); // Actually, the logic here seems to be inverted...
    interruptFlag = false;
  }
}

void isr() {
  interruptFlag = true;
}