
#include "CubeCell_NeoPixel.h"
CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);

int red, green, blue;

void setup() {
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); //SET POWER
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to ‘off’
  red = 255;
  green = 0;
  blue = 0;
}

void loop() {
  red = 255;
  green = 0;
  blue = 0;
  while ( green < 255 ) {
    pixels.setPixelColor(0, pixels.Color(red, ++green, blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  while ( red > 0 ) {
    pixels.setPixelColor(0, pixels.Color(--red, green, blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  while ( blue < 255 ) {
    pixels.setPixelColor(0, pixels.Color(red, green, ++blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  while ( green > 0 ) {
    pixels.setPixelColor(0, pixels.Color(red, --green, blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  while ( red < 255 ) {
    pixels.setPixelColor(0, pixels.Color(++red, green, blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
  while ( blue > 0 ) {
    pixels.setPixelColor(0, pixels.Color(red, green, --blue));
    pixels.show(); // Send the updated pixel colors to the hardware.
    delay(10); // Pause before next pass through loop
  }
}