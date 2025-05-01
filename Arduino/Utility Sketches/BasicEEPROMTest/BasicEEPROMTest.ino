/* 
 *  Test Write & Read from external [I2C) EEPROM
 */

#include "Wire.h"             // Need this to define second I2C bus (Only one bus on a Pro Mini?)
//#include "ProMiniPins.h"
//#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
#include "WiFiLoRa32V1Pins.h"
//#include "ESP32DevKitPins.h"

const int I2C_EEPROM_Address = 0x50;

uint8_t readPayload, writePayload;
uint16_t messageCounter = 0;

void setup() { 
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.print("[setup] ");
  Serial.println(" I2C EEPROM Test");
  Serial.println();

// Initialise EEPROM access

//  Wire.begin();
  Wire.begin(SDA,SCL);
  Wire.setClock(400000);

  Serial.println("[setup] Retrieve initial data from EEPROM...");
  readPayload = readEEPROMData(0);
  Serial.print("[setup] Retireved byte: ");
  Serial.println(readPayload, HEX);

  Serial.println("[setup] Set-up Complete");
  Serial.println(""); 
  writePayload = 0x12;

  //  Let everything settle down before continuing
  delay(500);
}

void loop() {
  writePayload++;
  Serial.print("[loop] Write byte: ");
  Serial.println(writePayload);
//  Store byte in EEPROM
  writeEEPROMData(writePayload,0);
  Serial.println("[loop] Byte written");
  delay(1000);

  Serial.println("[setup] Retrieve Counter...");
  readPayload = readEEPROMData(0);

  Serial.println();
  delay(5000);
}

void writeEEPROMData(uint8_t payload, long eeAddress)
{

  Wire.beginTransmission(I2C_EEPROM_Address);

  Wire.write((int)(eeAddress >> 8)); // MSB
  Wire.write((int)(eeAddress & 0xFF)); // LSB
//  Wire.write( eeAddress );

  //Write byte to EEPROM
  Wire.write(payload); //Write the data
  Serial.print( "[writeEEPROMData] Byte : " );
  Serial.println( payload );
  Wire.endTransmission(); //Send stop condition
}

uint8_t readEEPROMData(long eeaddress)
{
  Wire.beginTransmission(I2C_EEPROM_Address);

  Wire.write((int)(eeaddress >> 8)); // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
//  Wire.write( eeaddress );

  Wire.endTransmission();

  Wire.requestFrom(I2C_EEPROM_Address, 1);
  delay(1);
  
  byte rdata = 0xFF;
  if (Wire.available()){
    rdata = Wire.read();
    Serial.print( "[readEEPROMData] Byte: " );
    Serial.println( rdata );
  } else {
    Serial.println( "[readEEPROMData] Nothing to read..." );
  }
  return rdata;
}
