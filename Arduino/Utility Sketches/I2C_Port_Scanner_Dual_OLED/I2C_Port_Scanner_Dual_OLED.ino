/*
 * Had to include the OLED display stuff to make this work on the Heltec WiFi LoRa 32 modules.
 * We might need to use some conditional compile clauses to keep things simple for other MCUs.
 */

#include <SSD1306.h>            // OLED display
#include <Wire.h>
//#include <WiFiLoRa32V1Pins.h>   // Heltec WiFi LoRa 32 V1 pin definitions
//#include <WiFiLoRa32V2Pins.h>   // Heltec WiFi LoRa 32 V2 pin definitions
//#include <WiFiLoRa32V3Pins.h>   // Heltec WiFi LoRa 32 V3 pin definitions
//#include <ESP32DevKitPins.h>   // Elecrow ESP32S WiFi/BLE Board pin definitions
//#include <NodeMCUPins.h>   // Amica NodeM<CU pin definitions
//#include <ESP32Pins.h>   // ESP32-WROOM-32 pin definitions
//#include <ESP12Pins.h>   // ESP12-E/F pin definitions

#define SDA 19
#define SCL 20

SSD1306 display(0x3c, SDA_OLED, SCL_OLED);
SSD1306 display2(0x3c, SDA, SCL, GEOMETRY_128_64, I2C_TWO);

void setup()
{
//  Wire.begin(); // SDA,SCL
//  Wire1.begin(SDA,SCL); // SDA,SCL
 
  Serial.begin(115200);
  delay(500);
  Serial.println("\nI2C Scanner");
  Serial.print("SDA : ");
  Serial.print(SDA);
  Serial.print(", SCL: ");
  Serial.println(SCL);
  Serial.println();

// Initialise the OLED display
  
  Serial.println("[setup] Initialise display...");
  pinMode(RST_OLED,OUTPUT);     // GPIO16
  digitalWrite(RST_OLED,LOW);  // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  display.init();
  display.clear();
//  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5,15,"I2C Display 1");
  display.display();

 display2.init();
  display2.clear();
//  display.flipScreenVertically();
  display2.setFont(ArialMT_Plain_16);
  display2.setTextAlignment(TEXT_ALIGN_LEFT);
  display2.drawString(5,15,"I2C Display 2");
  display2.display();

}

 
void loop()
{
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
 
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // wait 5 seconds for next scan
}
