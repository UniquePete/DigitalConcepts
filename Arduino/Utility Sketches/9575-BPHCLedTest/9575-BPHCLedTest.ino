/*
Test 9575-BPHC LEDs
*/
#include <LoRa_APP.h>  // Included to test NeoPixel interaction with Vext
/*
NeoPixel has nothing to do with Vext. The observation will simply have been because Vext is raised to send packets. The fact that the NeoPixel is also flashed when sending a packet made it look like the two were related more directly than they actually are.
*/

#define pump_Ctrl GPIO5

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] 9575-BHC LED Test");
  Serial.println("[setup] LEDs : Vdd");
  Serial.println("               Vext");
  Serial.println("               pump_Ctrl");

  pinMode(Vext, OUTPUT);
  Serial.println("[setup] Vext LOW");
  digitalWrite(Vext, LOW);  // Turn the external power supply ON

  pinMode(pump_Ctrl, OUTPUT);
  Serial.println("[setup] pump_Ctrl LOW");
  digitalWrite(pump_Ctrl, LOW);  // Turn the pump OFF
  Serial.println();
}

void loop() {
  Serial.println("[loop] NeoPixel OFF");
  //  turnOffRGB();

  Serial.println("[loop] Vext HIGH");
  digitalWrite(Vext, HIGH);
  delay(1000);

  Serial.println("[setup] pump_Ctrl HIGH");
  digitalWrite(pump_Ctrl, HIGH);
  delay(1000);

  Serial.println("[loop] Vext LOW");
  digitalWrite(Vext, LOW);
  delay(1000);

  Serial.println("[setup] pump_Ctrl LOW");
  digitalWrite(pump_Ctrl, LOW);
  delay(1000);
  Serial.println();

  Serial.println("[loop] NeoPixel SEND");
  turnOnRGB(COLOR_SEND, 1);

  Serial.println("[loop] Vext HIGH");
  digitalWrite(Vext, HIGH);
  delay(1000);

  Serial.println("[setup] pump_Ctrl HIGH");
  digitalWrite(pump_Ctrl, HIGH);
  delay(1000);

  Serial.println("[loop] Vext LOW");
  digitalWrite(Vext, LOW);
  delay(1000);

  Serial.println("[setup] pump_Ctrl LOW");
  digitalWrite(pump_Ctrl, LOW);
  delay(1000);
  Serial.println();

  Serial.println("[loop] NeoPixel RECEIVED");
  turnOnRGB(COLOR_RECEIVED, 1);

  Serial.println("[loop] Vext HIGH");
  digitalWrite(Vext, HIGH);
  delay(1000);

  Serial.println("[setup] pump_Ctrl HIGH");
  digitalWrite(pump_Ctrl, HIGH);
  delay(1000);

  Serial.println("[loop] Vext LOW");
  digitalWrite(Vext, LOW);
  delay(1000);

  Serial.println("[setup] pump_Ctrl LOW");
  digitalWrite(pump_Ctrl, LOW);
  delay(1000);
  Serial.println();
}
