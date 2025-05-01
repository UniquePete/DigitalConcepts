/* LoRa Sender Node
 *
 * Function:
 * Assemble packets using the PacketHandler and forward data to the nominated Gateway Node
 * 
 * Description:
 * 1. Test external battery monitor
 *
 * 11 Dec 2022 v1.0 Code sample for 10068-BHWL3 (and similar)
 * 
 * 11 Dec 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

//#include "ESP32Pins.h"        // ESP32 Pin Definitions
#include "WiFiLoRa32V3Pins.h"     // Heltec WiFi LoRa 32 V3 Pin Definitions

#define batteryMonitor    A4a_A0
#define wakePin           A4a_A1

// ADC
static const float voltageFactor = (3.2/4096)*(127.0/100)*(4.20/4.20); // (max. ADC voltage/ADC range) * voltage divider factor * calibration factor
int inputValue = 0;
uint16_t batteryVoltage = 0;

void setup() {
  pinMode(wakePin,OUTPUT);         // Used to activate battery monitor 
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.println("[setup] Battery Monitor Test");
  Serial.println();

  //  Let everything settle down before continuing
  delay(100);
}

void loop() {
  // Read the battery voltage and send the status message off to the gateway
  readBatteryVoltage();  
  delay(5000);    // Report every 5 seconds
}

void readBatteryVoltage() {
  static const float voltageFactor = (3.2/4096)*(133.0/100)*(4.14/4.20); // (max. ADC voltage/ADC range) * voltage divider factor * calibration factor
  uint16_t inputValue = 0, peakValue = 0;
  Serial.println("[readBatteryLevel] Read ADC...");
  
  digitalWrite(wakePin,HIGH);
  delay(900);
  for (int i = 0; i < 10; i++) {
    /*
     * The ESP32 seems not to have a particularly good ADC, so we take 10 readings and choose the peak value
     * (becasue multiple values seem typically to follow a sine wave and we currently calibrate to the peak value)
     */
    delay(10);
    // and read ADC (Note the actual values of the voltage divider resistors in monitoring circuit)
    inputValue = analogRead(batteryMonitor); 
//    Serial.print(String(inputValue) + " = ");
//    Serial.println(String(inputValue*voltageFactor) + "V");
    if (inputValue > peakValue) peakValue = inputValue;
  }
  Serial.print(String(peakValue) + " = ");
  Serial.println(String(peakValue*voltageFactor) + "V");
  digitalWrite(wakePin,LOW);
}