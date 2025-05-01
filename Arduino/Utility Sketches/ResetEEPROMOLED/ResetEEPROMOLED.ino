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

#include <SSD1306.h>            // OLED display
#include "Wire.h"
#include "LangloEEPROM.h"     // Defines the payload Struct and EEPROM locations of specific parameters
//#include "ProMiniPins.h"
//#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
//#include "WiFiLoRa32V2Pins.h"
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

SSD1306 display(0x3c, SDA_OLED, SCL_OLED);

const int I2C_EEPROM_Address = 0x50;
#if defined(__ASR_Arduino__)    // The CubeCell uses a specific library (only supports 32K and 64K EEPROMs)
  EEPROM_AT24C32N bhcEEPROM;
#endif

LE_EEPROMPayload readData, writeData;

void setup() { 
#if defined(__ASR_Arduino__)
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); //SET POWER
#endif
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(50);
  Serial.println();
  Serial.print("[setup] ");
  Serial.print(" I2C EEPROM Read/Write Utility – ");
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Serial.println("ESP8266/ESP32");
#elif defined(__AVR_ATmega328P__)
  Serial.println("ATmega328P (Pro Mini)");
#elif defined(__ASR_Arduino__)
  Serial.println("CubeCell [Plus]");
#endif
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
  display.drawString(5,15,"EEPROM Test");
  display.display();

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

  delay(500);

  Serial.println("\nI2C Scan");
  Serial.print("SDA : ");
  Serial.print(SDA);
  Serial.print(", SCL: ");
  Serial.println(SCL);
  Serial.println();

// Scan the I2C bus

  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
 
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
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

  if (smartSerial) {
    Serial.println("[setup] Smart Serial Addressing (32K+ EEPROM)");
  } else {
    Serial.println("[setup] Standard Serial Addressing (16K- EEPROM)");
  }
  
  Serial.println(F("[setup] Retrieve data from EEPROM..."));
  eepromReadBytes(readData.payloadByte, LE_sequenceBytes, LE_sequenceLocation);
  Serial.print("[setup]   Sequence #: ");
  Serial.println(readData.counter);
  eepromReadBytes(readData.payloadByte, LE_rainBytes, LE_rainLocation);
  Serial.print("[setup] Rain Counter: ");
  Serial.println(readData.counter);
  eepromReadBytes(readData.payloadByte, LE_windBytes, LE_windLocation);
  Serial.print("[setup] Wind Counter: ");
  Serial.println(readData.counter);
  promptForInput();
}

void loop() {
  if ( Serial.available() > 0 ) {
    inputString = Serial.readString(); // read the command
//    Serial.print("[loop] Input string: ");
//    Serial.println(inputString);
    processInput( inputString );
    promptForInput();
  }
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

    eepromWriteBytes(parameterLocation, parameterBytes, writeData.payloadByte);

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
      eepromReadBytes(readData.payloadByte, LE_sequenceBytes, LE_sequenceLocation);
      Serial.print("  Sequence # (SN): ");
      Serial.println(readData.counter);
      eepromReadBytes(readData.payloadByte, LE_rainBytes, LE_rainLocation);
      Serial.print("Rain Counter (RC): ");
      Serial.println(readData.counter);
      eepromReadBytes(readData.payloadByte, LE_windBytes, LE_windLocation);
      Serial.print("Wind Counter (WC): ");
      Serial.println(readData.counter);
    } else {
      eepromReadBytes(readData.payloadByte, parameterBytes, parameterLocation);
      Serial.println(readData.counter);
    }
  }
 }

void eepromReadBytes(char* buf, uint8_t byteCount, long eepromAddress) {
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

  Wire.beginTransmission(I2C_EEPROM_Address);
  if (smartSerial) {
    Wire.write((int)(eepromAddress >> 8)); // MSB
    Wire.write((int)(eepromAddress & 0xFF)); // LSB
  } else {
    Wire.write( eepromAddress );      
  }
  Wire.endTransmission();
  
  byte rdata = 0xFF;
  for (int x = 0 ; x < byteCount ; x++) {    
    Wire.requestFrom(I2C_EEPROM_Address, 1);
    delay(5);
    
    if (Wire.available()) {
      rdata = Wire.read();
//      Serial.print( "[readEEPROMData] Byte: " );
//      Serial.println( rdata );
      buf[x] = rdata;
    } else {
//      Serial.println(F("[eepromReadBytes] Nothing to read..."));
    }
  }
#endif
}

void eepromWriteBytes(long eepromAddress, uint8_t byteCount, char* buf) {
#if defined(__ASR_Arduino__)
    bhcEEPROM.WriteBytes(eepromAddress, byteCount, buf);
#else

    Serial.print("[eepromWriteBytes] Writing EEPROM at location: ");
    Serial.println(eepromAddress);
    Serial.print("[eepromWriteBytes] Bytes to write: ");
    Serial.println(byteCount);
    Serial.print("  Byte 0: 0x");
    Serial.println(buf[0],HEX);
    Serial.print("  Byte 1: 0x");
    Serial.println(buf[1],HEX);

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
#endif
}
