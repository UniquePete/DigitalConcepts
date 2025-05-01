/*
  TPL5010 Watchdog Timer test

  Function:
  Loop, generating time stamps, to identify the watchdog timer timeout interval.
  Since the dog would be patted every time a report is sent out, the timeout interval
  should be set to allow for a normal cycle, including the time taken to read sensors,
  the reporting interval and a little overhead.

  1 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au
*/

#include <Arduino.h>
#include <HT_SH1107Wire.h>     // CubeCell Plus display library
#include <CubeCellPlusPins.h>   // Heltec CubeCell Plus pin definitions

#define done GPIO12

unsigned long startTime, nowTime, timeInterval;

// OLED Display
SH1107Wire  myDisplay(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr, freq, sda, scl, resolution, rst

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] 10068-BHCP TPL5010 Watchdog Timer Test");

  myDisplay.init();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.screenRotate(ANGLE_180_DEGREE);

  pinMode(done, OUTPUT);
  digitalWrite(done,LOW);
  Serial.println("[setup] Pat the dog...");
  digitalWrite(done,HIGH);
  digitalWrite(done,LOW);

  startTime = millis();
}

void loop() {
  nowTime = millis();
  timeInterval = (nowTime - startTime)/1000;

  myDisplay.clear();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  myDisplay.drawString(5,5,"TPL5010");
  myDisplay.drawString(5,20,"Watchdog Timer Test");
  myDisplay.setFont(ArialMT_Plain_24);
  myDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
  myDisplay.drawString(120,35,String(timeInterval));
  myDisplay.display();

  Serial.print("[loop] ");
  Serial.print(timeInterval);
  Serial.println(" sec");
  delay(973);  // This works out to provide close to one cycle around the loop every second
}