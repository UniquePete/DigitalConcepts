/* 
 *  (Re)set/Display [I2C] EEPROM contents
 *  Script to set, reset or display the following counters stored in EEPROM:
 *  1. Sequence # (2-byte value stored at [byte] address 0
 *  2. Rainfall counter (2-byte value stored at [byte] address 4
 *  3. Wind speed counter (2-byte value stored at [byte] adress 8
 *  
 *  Note: EEPROM size is measured in bits (2Kb EEPROM is 256KB), but address is byte offset from 0
 *  
 * 15 Aug 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#elif defined(__AVR_ATmega328P__)
#elif defined(__ASR_Arduino__)
  #include <AT24C32N.h>
#endif

#include "Wire.h"
#include "LangloEEPROM.h"     // Defines the payload Struct and EEPROM locations of specific parameters
//#include "ProMiniPins.h"
//#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
//#include "WiFiLoRa32V2Pins.h"
//#include "WiFiLoRa32V3Pins.h"
//#include "ESP32DevKitPins.h"
//#include "CubeCellPins.h"
#include "CubeCellPlusPins.h"

/*
 * LangloEEPROM.h defines:
 * #define LE_sequenceLocation    0
 * #define LE_rainLocation        4
 * #define LE_windLocation        8
 *
 * #define LE_sequenceBytes       2
 * #define LE_rainBytes           2
 * #define LE_windBytes           2
 *
 * union LE_EEPROMPayload {
 *   char payloadByte[2];
 *   uint16_t counter;
 * };
 */

/* The following are the commends that will be recognised */
const char SN[]         = "SN"; // Sequence Number
const char RC[]         = "RC"; // Rain Counter
const char WC[]         = "WC"; // Wind Counter
const char standard[]   = "ST"; // Standard Addressing Mode (1 byte)
const char smart[]      = "SM"; // Smart Addressing Mode (2 byte)
const char returnChar[] = "";   // Carriage Return - Display current values

String inputString;
char identifier[3];
int identifierValue;
uint16_t parameterBytes;
uint16_t parameterLocation;
bool smartSerial = false;       // For 32K and 64K EEPROMs
bool writeEEPROM = false;

const int I2C_EEPROM_Address = 0x50;
#if defined(__ASR_Arduino__)    // The CubeCell uses a specific library (only supports 32K and 64K EEPROMs)
  EEPROM_AT24C32N bhcEEPROM;
#endif

LE_EEPROMPayload readData = {0}, writeData = {0};

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
  delay(3000);    // The Pro Mini seems to want to reboot after 2 seconds, but then it's OK, so don't do any until that's all settled
#elif defined(__ASR_Arduino__)
  delay(500);
#endif
 
  setEepromAddressing();

  if (smartSerial) {
    Serial.println("[setup] Using Smart Serial Addressing (32K+ EEPROM)");
  } else {
    Serial.println("[setup] Using Standard Serial Addressing (16K- EEPROM)");
  }
  
  Serial.println(F("[setup] Retrieve data from EEPROM..."));
  getCurrentData();      
  promptForInput();
}

void loop() {
  if ( Serial.available() > 0 ) {
    inputString = Serial.readString(); // read the command
    Serial.print("[loop] Input string: ");
    Serial.println(inputString);
    processInput( inputString );
    promptForInput();
  }
}

void setEepromAddressing() {
  bool smartSerialState;
  byte writeTestBuf[2] = {0x1,0x0};
  byte readTestBuf[2] = {0x0};
  byte stdBuf[2], smtBuf[2];
  
// Save the smartSerial setting

  smartSerialState = smartSerial;

// Save the EEPROM content of the area we're about to use
// Use both addressing methods to retrieve the contents. When we know which addressing
// method is the correct one, we'll use that to restore the value that was retrieved using
// that method.

//  Serial.println(F("[setEepromAddressing] Saving current value..."));
//  Serial.println(F("[setEepromAddressing] Read using Standard Addressing..."));
  smartSerial = false;
  eepromReadBytes(stdBuf,2,0);
/*
  Serial.print("[setEepromAddressing] Byte 0 : 0x");
  Serial.println(stdBuf[0],HEX);
  Serial.print("[setEepromAddressing] Byte 1 : 0x");
  Serial.println(stdBuf[1],HEX);
*/
//  Serial.println(F("[setEepromAddressing] Read using Smart Addressing..."));
  smartSerial = true;
  eepromReadBytes(smtBuf,2,0);
/*
  Serial.print("[setEepromAddressing] Byte 0 : 0x");
  Serial.println(smtBuf[0],HEX);
  Serial.print("[setEepromAddressing] Byte 1 : 0x");
  Serial.println(smtBuf[1],HEX);
*/
// Now determine whether or not we need to use smart addressing
// First, write using Smart Addressing

//  Serial.println(F("[setEepromAddressing] Writing test value using Smart Addresing..."));
  eepromWriteBytes(writeTestBuf,1,0);
  
// Next, read using standard addressing

  smartSerial = false;
//  Serial.println(F("[setEepromAddressing] Reading test value..."));
  eepromReadBytes(readTestBuf,1,0);
  
// If EEPROM required Smart Addressing, we will read what we wrote: "1". If "0" is read,
// then Standard Addressing must be required.
  if ( readTestBuf[0] == 0x0 ) {
//    Serial.println(F("[setEepromAddressing] EEPROM: 16K-, Use Standard Addressing"));
    smartSerial = false;
//    Serial.println(F("[setEepromAddressing] Restoring saved value..."));
    eepromWriteBytes(stdBuf,2,0);
  } else if ( readTestBuf[0] == 0x1 ) {
//    Serial.println(F("[setEepromAddressing] EEPROM: 32K+, Use Smart Addressing"));
    smartSerial = true;
//    Serial.println(F("[setEepromAddressing] Restoring saved value..."));
    eepromWriteBytes(smtBuf,2,0);
  } else {
    Serial.println(F("[setEepromAddressing] Unexpected result..."));
  }

  Serial.println(F("[setEepromAddressing] Check complete"));
  smartSerial = smartSerialState;
  delay(100);     // Not sure why, but we need a litte pause here or the next read is hosed
}

void promptForInput() {
  Serial.println();
  Serial.println("Enter  STandard or SMart to set Standard (16K-) or Smart (32K+) Addressing");
  Serial.println("       <Return> to display current EEPROM content");
  Serial.println("       SN, RC or WC = <number> to set Sequence #, Rain Counter or Wind Counter");
  Serial.println();
}

void processInput( String inputString ) {
  int i = 0;
//  Serial.print("[processInput] String: ");
//  Serial.println(inputString);
  inputString.toUpperCase();
//  Serial.print("[processInput] Convert to Upper Case: ");
//  Serial.println(inputString);
  
  String character;
  int charVal;
  int stringLength = inputString.length();

  char buf[50];
  inputString.toCharArray(buf,50);
//  Serial.print("[processInput] Convert to Char array: ");
//  Serial.println(buf);
  
//  Serial.println("Skip leading white space...");
  i = 0;
  while ( isspace(buf[i]) ) i++;
/*
  Serial.print("At position ");
  Serial.println(i);

  Serial.println("Extract identifier...");
  Serial.print("Substring: ");
  Serial.println(inputString.substring(i,i+2));
  Serial.println("Extract to Char array...");
*/
  inputString.substring(i,i+2).toCharArray(identifier,3);
/*
  Serial.print("Identifier: ");
  Serial.print(identifier);
  Serial.print(" (");
  Serial.print(identifier[0],HEX);
  Serial.print(identifier[1],HEX);
  Serial.print(identifier[2],HEX);
  Serial.println(")");
  Serial.print("Choose from: ");
  Serial.print(SN);
  Serial.print(" (");
  Serial.print(SN[0], HEX);
  Serial.print(SN[1], HEX);
  Serial.print(SN[2], HEX);
  Serial.print("), ");
  Serial.print(RC);
  Serial.print(" (");
  Serial.print(RC[0], HEX);
  Serial.print(RC[1], HEX);
  Serial.print(RC[2], HEX);
  Serial.print("), ");
  Serial.print(WC);
  Serial.print(" (");
  Serial.print(WC[0], HEX);
  Serial.print(WC[1], HEX);
  Serial.print(WC[2], HEX);
  Serial.println(")");
*/
  if ( strcmp(identifier,SN) == 0 ) {
    parameterBytes = LE_sequenceBytes;
    parameterLocation = LE_sequenceLocation;
    writeEEPROM = true;
//    Serial.println("Identifier is SN");
  } else if ( strcmp(identifier,RC) == 0 ) {
    parameterBytes = LE_rainBytes;
    parameterLocation = LE_rainLocation;
    writeEEPROM = true;
//    Serial.println("Identifier is RC");
  } else if ( strcmp(identifier,WC) == 0 ) {
    parameterBytes = LE_windBytes;
    parameterLocation = LE_windLocation;
    writeEEPROM = true;
//    Serial.println("Identifier is WC");
  } else {
    writeEEPROM = false;
    if ( strcmp( identifier,standard ) == 0 ) {
      smartSerial = false;
      Serial.println("Set Standard [1 byte] Addressing");
    } else if ( strcmp( identifier,smart ) == 0 ) {
      smartSerial = true;      
      Serial.println("Set Smart [2 byte] Addressing");
    } else {
      if ( strcmp( identifier,returnChar ) != 0 ) {
        Serial.print("Command <");
        Serial.print(identifier);
        Serial.println("> not recognised");        
      }
    }
  }

  i = 0;
  while ( !isdigit(buf[i]) && i < stringLength ) i++;
  if ( i < stringLength ) {
//    Serial.print("Got a digit at position ");
//    Serial.println(i);
    int j = i;
    while ( isdigit(buf[j]) && j < stringLength ) j++;
//    Serial.print("Found integer at postions ");
//    Serial.print(i);
//    Serial.print(" to ");
//    Serial.println(j-1);
  
    identifierValue = inputString.substring(i,j).toInt();
//    Serial.print("Integer value: ");
//    Serial.println(identifierValue);
    writeData.counter = identifierValue;
    if (smartSerial) {
      Serial.println("Smart Serial Addressing (32K+ EEPROM)");
    } else {
      Serial.println("Standard Serial Addressing (16K- EEPROM)");
    }
    Serial.print("Write ");
    Serial.print(identifier);
    Serial.print(" = ");
    Serial.print(identifierValue);
    Serial.println(" to EEPROM...");

    eepromWriteBytes(writeData.payloadByte, parameterBytes, parameterLocation);

  } else {
//    Serial.println("No digits, just return the current value...");
    bool readAll = false;
    if ( strcmp(identifier,SN) == 0 ) {
      parameterBytes = LE_sequenceBytes;
      parameterLocation = LE_sequenceLocation;
      Serial.print("  Sequence #: ");
    } else if ( strcmp(identifier,RC) == 0 ) {
      parameterBytes = LE_rainBytes;
      parameterLocation = LE_rainLocation;
      Serial.print("Rain Counter: ");
    } else if ( strcmp(identifier,WC) == 0 ) {
      parameterBytes = LE_windBytes;
      parameterLocation = LE_windLocation;
      Serial.print("Wind Counter: ");
    } else {
//      Serial.println("Unrecognised identifier, return them all");
      readAll = true;
    }
    if ( readAll ) {
      if (smartSerial) {
        Serial.println("Smart Serial Addressing (32K+ EEPROM)");
      } else {
        Serial.println("Standard Serial Addressing (16K- EEPROM)");
      }
      getCurrentData();      
    } else {
      eepromReadBytes(readData.payloadByte, parameterBytes, parameterLocation);
      Serial.println(readData.counter);
    }
  }
 }

void getCurrentData() {
  eepromReadBytes(readData.payloadByte, LE_sequenceBytes, LE_sequenceLocation);
  Serial.print("  Sequence # (SN): ");
  Serial.println(readData.counter);
  eepromReadBytes(readData.payloadByte, LE_rainBytes, LE_rainLocation);
  Serial.print("Rain Counter (RC): ");
  Serial.println(readData.counter);
  eepromReadBytes(readData.payloadByte, LE_windBytes, LE_windLocation);
  Serial.print("Wind Counter (WC): ");
  Serial.println(readData.counter);
}

void eepromReadBytes(byte* buf, uint8_t byteCount, long eepromAddress) {
  /*
#if defined(__ASR_Arduino__)
//  Serial.println("[eepromReadBytes] Reading... ");
//  Serial.print("  Byte Count: ");
//  Serial.println(byteCount);
//  Serial.print("  EEPROM Address: ");
//  Serial.println(eepromAddress);
  bhcEEPROM.RetrieveBytes(buf, byteCount, eepromAddress, false);
//  Serial.print("  Read: ");
//  Serial.println(readData.counter);
#else
*/
  Wire.beginTransmission(I2C_EEPROM_Address);
  if (smartSerial) {
//    Serial.println( "[readEEPROMData] Using Smart Serial Addressing..." );
    Wire.write((int)(eepromAddress >> 8)); // MSB
    Wire.write((int)(eepromAddress & 0xFF)); // LSB
  } else {
//    Serial.println( "[readEEPROMData] Using Standard Serial Addressing..." );
    Wire.write( eepromAddress );      
  }
  Wire.endTransmission();
  
  byte rdata = 0x0;
  for (int x = 0 ; x < byteCount ; x++) {    
    Wire.requestFrom(I2C_EEPROM_Address, 1);
    delay(5);
    
    if (Wire.available()) {
      rdata = Wire.read();
      buf[x] = rdata;
/*      
      Serial.print( "[readEEPROMData] Byte : 0x" );
      Serial.println( rdata,HEX );
      Serial.print( "[readEEPROMData] Buf Byte : 0x" );
      Serial.println( buf[x],HEX );
*/
    } else {
//      Serial.println(F("[eepromReadBytes] Nothing to read..."));
    }
  }
//#endif
}

void eepromWriteBytes(byte* buf, uint8_t byteCount, long eepromAddress) {
  /*
#if defined(__ASR_Arduino__)
    bhcEEPROM.WriteBytes(eepromAddress, byteCount, buf);
#else
/*
    Serial.print("[eepromWriteBytes] Writing EEPROM at location: ");
    Serial.println(eepromAddress);
    Serial.print("[eepromWriteBytes] Bytes to write: ");
    Serial.println(byteCount);
    Serial.print("  Byte 0: 0x");
    Serial.println(buf[0],HEX);
    Serial.print("  Byte 1: 0x");
    Serial.println(buf[1],HEX);
*/
    Wire.beginTransmission(I2C_EEPROM_Address);
    if (smartSerial) {
      Wire.write((int)(eepromAddress >> 8));    // MSB
      Wire.write((int)(eepromAddress & 0xFF));  // LSB
    } else {
      Wire.write(eepromAddress);
    }

    for (byte x = 0 ; x < byteCount ; x++) {
      Wire.write(buf[x]);                      // Write the data
      /*
      Serial.print( "[processInput] Byte " );
      Serial.print( x );
      Serial.print( ": 0x" );
      Serial.println( buf[x],HEX );
      */
    }
    Wire.endTransmission();                   // Send stop condition
//#endif
}
