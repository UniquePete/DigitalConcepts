/* 
 *  Determine required EEPROM addressing mode
 *  
 *  Note: EEPROM size is measured in bits (a 2K EEPROM comprises 256 Bytes),
 *  but address is byte offset from 0
 *  
 * 22 Feb 2023
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include "Wire.h"
//#include "ProMiniPins.h"
#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
//#include "WiFiLoRa32V2Pins.h"
//#include "WiFiLoRa32V3Pins.h"
//#include "ESP32DevKitPins.h"
//#include "CubeCellPins.h"
//#include "CubeCellPlusPins.h"

#define _EH_DEBUG false

uint16_t parameterBytes;
uint16_t parameterLocation;
bool smartSerial = false;       // For 32K and 64K EEPROMs
bool writeEEPROM = false;

const int I2C_EEPROM_Address = 0x50;

void setup() { 
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Serial.begin(115200);
#elif defined(__AVR_ATmega328P__)
  Serial.begin(9600);     // Pro Mini FTDI Adaptor can't handle high baud rates
#elif defined(__ASR_Arduino__)
  Serial.begin(115200);
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); //SET POWER
#endif
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(50);
  Serial.println();
  Serial.print("[setup] ");
  Serial.print(" I2C EEPROM Read/Write Utility : ");
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Serial.println("ESP8266/ESP32");
#elif defined(__AVR_ATmega328P__)
  Serial.println("ATmega328P (Pro Mini)");
#elif defined(__ASR_Arduino__)
  Serial.println("CubeCell [Plus]");
#endif
  Serial.println();

// Initialise EEPROM access

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Wire.begin(SDA,SCL);
  Wire.setClock(400000);
#elif defined(__AVR_ATmega328P__)
  Wire.begin();
  Wire.setClock(400000);
#elif defined(__ASR_Arduino__)
  Wire.begin();
#endif

//  Let everything settle down before continuing

#if defined(__AVR_ATmega328P__)
  delay(3000);
#elif defined(__ASR_Arduino__)
 delay(500);
#endif
 
  checkEepromAddressing();
 }

void loop() {
}

void checkEepromAddressing() {
  bool smartSerialState;
  uint8_t stdBuf[2], smtBuf[2], readTestBuf[2], writeTestBuf[2];
  
  readTestBuf[0] = 0x0;
  writeTestBuf[0] = 0x1;
  writeTestBuf[1] = 0x1;
  
// Save the smartSerial setting

  smartSerialState = smartSerial;

// Save the EEPROM content of the area we're about to use
// Use both addressing methods to retrieve the contents. When we know which addressing
// method is the correct one, we'll use that to restore the value that was retrieved using
// that method.

  Serial.println(F("[checkEepromSize] Saving current value..."));

  if ( _EH_DEBUG ) { Serial.println(F("[checkEepromSize] Using Standard Addressing...")); }
  smartSerial = false;
  eepromReadBytes(stdBuf,2,0);

  if ( _EH_DEBUG ) { Serial.println(F("[checkEepromSize] Using Smart Addressing...")); }
  smartSerial = true;
  eepromReadBytes(smtBuf,2,0);

// Now determine whether or not we need to use smart addressing
// First, write using Smart Addressing

  Serial.println(F("[checkEepromSize] Smart Write test value..."));
  eepromWriteBytes(writeTestBuf,1,1);
  eepromWriteBytes(writeTestBuf,1,0);
  
// Next, read using Standard addressing

  Serial.print("[checkEepromSize] smartSerial (true?): 0x");
  Serial.println(smartSerial);
  smartSerial = false;
  Serial.print("[checkEepromSize] smartSerial (false): 0x");
  Serial.println(smartSerial);
  Serial.println(F("[checkEepromSize] Standard Read test value..."));
  eepromReadBytes(readTestBuf,1,0);
  Serial.print(F("[checkEepromSize] Test Value : 0x"));
  Serial.println(readTestBuf[0],HEX);
 
// If EEPROM required Standard Addressing, we will read what was just written to location 0000, i.e. "0".
// If not, Smart Addressing must be required.

  if ( readTestBuf[0] == 0x0 ) {
    Serial.println(F("[checkEepromSize] EEPROM: 16K-, Standard Addressing"));
    if ( _EH_DEBUG ) { Serial.println(F("[checkEepromSize] Restoring saved value...")); }
    smartSerial = false;
    eepromWriteBytes(stdBuf,2,0);
  } else if ( readTestBuf[0] == 0x1 ) {
    Serial.println(F("[checkEepromSize] EEPROM: 32K+, Smart Addressing"));
    if ( _EH_DEBUG ) { Serial.println(F("[checkEepromSize] Restoring saved value...")); }
    smartSerial = true;
    eepromWriteBytes(smtBuf,2,0);
  } else {
    Serial.print(F("[checkEepromSize] Unrecognised entry : 0x"));
    Serial.println(readTestBuf[0],HEX);
  }
  Serial.println(F("[checkEepromSize] Check complete"));
  smartSerial = smartSerialState;
}

void eepromReadBytes(uint8_t* buf, uint8_t byteCount, uint16_t eepromAddress) {
  int msb, lsb, addr;
  Wire.beginTransmission(I2C_EEPROM_Address);
  if (smartSerial) {
//    Serial.print("[eepromReadBytes] smartSerial true :");
//    Serial.println(smartSerial);
    msb = (int)(eepromAddress >> 8);
//    Serial.print("[eepromReadBytes] MSB : 0x");
//    Serial.println(msb);
    Wire.write(msb); // MSB
    lsb = (int)(eepromAddress & 0xFF);
//    Serial.print("[eepromReadBytes] LSB : 0x");
//    Serial.println(lsb);
    Wire.write(lsb); // LSB
    if ( _EH_DEBUG ) {
      Serial.print(F("[eepromReadBytes] Address HByte : 0x"));
      Serial.println((int)(eepromAddress >> 8),HEX);
      Serial.print(F("[eepromReadBytes] Address LByte : 0x"));
      Serial.println((int)(eepromAddress & 0xFF),HEX);
    }
   } else {
//    Serial.print("[eepromReadBytes] smartSerial false :");
//    Serial.println(smartSerial);
    addr = (int)eepromAddress;
//    Serial.print("[eepromReadBytes] Address : 0x");
//    Serial.println(addr);
    Wire.write(addr);      
    if ( _EH_DEBUG ) {
      Serial.print(F("[eepromReadBytes] Address Simple : 0x"));
      Serial.println(eepromAddress,HEX);
    }
  }
  Wire.endTransmission(); delay(10);
//    Serial.print("[eepromReadBytes] Request ");
//    Serial.print(byteCount);
//    Serial.print(" bytes from device 0x");
//    Serial.println(I2C_EEPROM_Address,HEX);
  Wire.requestFrom(I2C_EEPROM_Address, (int)byteCount); delay(10);
  
  uint8_t rdata;
  for (int x = 0 ; x < byteCount ; x++) {        
    if (Wire.available()) {
      rdata = Wire.read();
      buf[x] = rdata;
      if ( _EH_DEBUG ) {
        Serial.print( "[eepromReadBytes] Data Byte " );
        Serial.print( x );
        Serial.print( " : 0x" );
        Serial.println( rdata,HEX );
      }
    } else {
      if ( _EH_DEBUG ) { Serial.println(F("[eepromReadBytes] Nothing to read...")); }
    }
  }
}

void eepromWriteBytes(uint8_t* buf, uint8_t byteCount, uint16_t eepromAddress) {
  int msb, lsb, addr;
  if ( _EH_DEBUG ) {
    Serial.print("[eepromWriteBytes] Writing EEPROM at location: ");
    Serial.println(eepromAddress);
    Serial.print("[eepromWriteBytes] Bytes to write : ");
    Serial.println(byteCount);
  }
  Wire.beginTransmission(I2C_EEPROM_Address);
  if (smartSerial) {
//    Serial.print("[eepromWriteBytes] smartSerial true :");
//    Serial.println(smartSerial);
    msb = (int)(eepromAddress >> 8);
    delay(10);
//    Serial.print("[eepromWriteBytes] MSB : 0x");
//    Serial.println(msb);
    Wire.write(msb); // MSB
    lsb = (int)(eepromAddress & 0xFF);
    delay(10);
//    Serial.print("[eepromWriteBytes] LSB : 0x");
//    Serial.println(lsb);
    Wire.write(lsb); // LSB
    if ( _EH_DEBUG ) {
      Serial.print("[eepromWriteBytes] Address HByte : 0x");
      Serial.println((int)(eepromAddress >> 8),HEX);
      Serial.print("[eepromWriteBytes] Address LByte : 0x");
      Serial.println((int)(eepromAddress & 0xFF),HEX);
    }
  } else {
//    Serial.print("[eepromReadBytes] smartSerial false :");
//    Serial.println(smartSerial);
    addr = (int)eepromAddress;
//    Serial.print("[eepromReadBytes] Address : 0x");
//    Serial.println(addr);
    Wire.write(addr);      
    if ( _EH_DEBUG ) {
      Serial.print("[eepromWriteBytes] Address Simple : 0x");
      Serial.println(eepromAddress,HEX);
    }
  }

  for (byte x = 0 ; x < byteCount ; x++) {
    Wire.write(buf[x]);                      // Write the data
    if ( _EH_DEBUG ) {
      Serial.print( "[eepromWriteBytes] Data Byte " );
      Serial.print( x );
      Serial.print( " : 0x" );
      Serial.println( buf[x],HEX );
    }
  }
  Wire.endTransmission();                   // Send stop condition
}
