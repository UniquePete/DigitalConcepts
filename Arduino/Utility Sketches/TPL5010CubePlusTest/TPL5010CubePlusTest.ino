/*
delay resistor ~11k ohm (~10 sec)

interval 3 sec
mod 2
(pat the dog every 6 sec)
OK

interval 5 sec
mod 2
(pat the dog every 10 sec)
FAIL (just misses after about 5 cycles)
*/

#include <Arduino.h>
#include <HT_SH1107Wire.h>     // CubeCell Plus display library
#include <CubeCellPlusPins.h>   // Heltec CubeCell Plus pin definitions

#define done GPIO12
#define interval 5
#define mod 2

int counter = 0;

// OLED Display
SH1107Wire  myDisplay(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr, freq, sda, scl, resolution, rst

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] CubeCell Plus TPL5010 Watchdog Timer Test");

  myDisplay.init();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.screenRotate(ANGLE_180_DEGREE);
    
  myDisplay.clear();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  myDisplay.drawString(5,5,"TPL5010 Watchdog Timer Test");
  myDisplay.display();
  delay(1000);

  pinMode(done, OUTPUT);
  digitalWrite(done,LOW);
}

void loop() {
  counter++;

  myDisplay.clear();
  myDisplay.drawString(5,10,"  Interval : " + String(interval));
  myDisplay.drawString(5,25,"       Mod : " + String(mod));
  myDisplay.drawString(5,40," Counter : " + String(counter));
  myDisplay.display();

  delay(1000);
  Serial.print("[loop] ");
  Serial.print(counter);
  Serial.println(" Looping...");
  if ( (counter % mod) == 0 ) {
    Serial.println("[loop] Pat the dog...");
    digitalWrite(done,HIGH);
    digitalWrite(done,LOW);
  }
  Serial.print("[loop] ");
  Serial.print(interval);
  Serial.println(" sec delay...");
  delay(interval*1000);
}
