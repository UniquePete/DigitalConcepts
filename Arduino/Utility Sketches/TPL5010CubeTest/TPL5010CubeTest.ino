/*
delay resistor ~11k ohm (~10 sec)
delay resistor ~30k ohm (~2 min)

interval 3 sec
mod 2
(pat the dog every 6 sec)
OK

interval 3 sec
mod 4
(pat the dog every 12 sec)
FAIL (misses after ~4 cycles)

Extended test to check for any conflict with RGB LED control
Conclusion: There is conflict! The NeoPixel steals GPIO4
*/

#include <Arduino.h>
#include <LoRa_APP.h>
#include <CubeCellPins.h>   // Heltec CubeCell pin definitions

#define done GPIO0
#define interval 3
#define mod 4
#define neoPixel true

int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] CubeCell TPL5010 Watchdog Timer Test");

  pinMode(done, OUTPUT);
  digitalWrite(done,LOW);
}

void loop() {
  counter++;
  Serial.print("[loop] ");
  Serial.print(counter);
  Serial.println(" Looping...");
  if ( (counter % mod) == 0 ) {
    Serial.println("[loop] Pat the dog...");
    digitalWrite(done,HIGH);
    digitalWrite(done,LOW);
    if ( neoPixel ) {
      turnOnRGB(COLOR_RECEIVED,0);
      turnOffRGB();
    }
  } else {
    if ( neoPixel ) {
      turnOnRGB(COLOR_SEND,0);
      turnOffRGB();
    }
  }
  Serial.print("[loop] ");
  Serial.print(interval);
  Serial.println(" sec delay...");
  delay(interval*1000);
}
