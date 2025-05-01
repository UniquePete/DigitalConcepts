/* (Re)set/Display [I2C] EEPROM contents
  
   Function:
   Set, reset or display parameters stored in EEPROM using the EepromHandler.
   This currently includes:
     1. Gateway MAC (4-byte uint32 value stored at [byte] address 0
     2. Node MAC (4-byte uint32 value stored at [byte] address 8
     3. Descriptor (32-byte 'string' stored at [byte] address 16
     4. Sequence # (2-byte uint16 value stored at [byte] address 128
     5. Rainfall counter (2-byte uint16 value stored at [byte] address 132
     6. Tank ID (1-byte uint8 value stored at [byte] address 136
     7. Pump ID (1-byte uint8 value stored at [byte] address 138
 
    This version also includes the ability to generate a HEX dump of the first 256 bytes
    of the EEPROM.
    
    Note: EEPROM size is measured in bits (2Kb EEPROM is 256KB), but address is byte offset from 0
    
    25 Feb 2023 1.0.0 Base release
    24 May 2023 1.1.0 Add software revision variables
                      Consolidate output generation
                      Turn on Vext for Heltec ESP32 V2 & V3 modules
                      Add more detailed board architecture descriptions
                1.1.1 Whatever library it is/was that includes the isHexadecimalDigit() and isPrintable()
                      functions seems to have disappeared (if it ever existed) from the CubeCell build
                      path, so appropriate substitute functions are now included
    13 Jan 2024 1.1.2 Add CubeCellV2Pins.h option
     3 Apr 2024 1.2.0 Add TANKID & PUMPID
     7 Apr 2024 1.2.1 Call setSmartSerial(), rather than getSmartSerial(), to determine what addressing method
                      is required, rather than just assuming that it will be Smart Addressing.
    7 Apr 2024
    Digital Concepts
    www.digitalconcepts.net.au
 */
 
#include "Wire.h"
#include "EepromHandler.h"        // EEPROM class with access methods
#include "ProMiniPins.h"          // Note required Serial Monitor baud rate: 9600
//#include "ESP12Pins.h"            // Used to define I2C SDA/SCL
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
//#include "WiFiLoRa32V2Pins.h"
//#include "WiFiLoRa32V3Pins.h"
//#include "ESP32DevKitPins.h"
//#include "CubeCellPins.h"
//#include "CubeCellV2Pins.h"
//#include "CubeCellPlusPins.h"

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {1,2,1};

/* The following are the commands that will be recognised */
const char GM[]         = "GM"; // Gateway MAC
const char NM[]         = "NM"; // Node MAC
const char DS[]         = "DS"; // Descriptor
const char SN[]         = "SN"; // Sequence Number
const char RC[]         = "RC"; // Rain Counter
const char TD[]         = "TD"; // Tank ID
const char PD[]         = "PD"; // Pump ID
const char HX[]         = "HX"; // HEX Dump EEPROM contents
const char SC[]         = "SC"; // Scrub EEPROM contents
const char standard[]   = "ST"; // Standard Addressing Mode (1 byte)
const char smart[]      = "SM"; // Smart Addressing Mode (2 byte)
const char returnChar[] = "";   // Carriage Return - Display current values

String inputString;
char identifier[3];
uint8_t identifier8Value;
uint16_t identifier16Value;
uint32_t identifier32Value;
EH_dataType parameter;
bool smartSerial = false;       // Smart Serial Addressing required for 32K and 64K EEPROMs
bool readOn = false;
bool printAll = false;

uint32_t gatewayMAC = 0;
uint32_t nodeMAC = 0;
uint8_t* descriptor;
// - - - - - - - - - -
uint8_t debugBuffer[8];
// - - - - - - - - - -

const uint8_t I2C_EEPROM_Address = 0x50;
uint16_t sequenceNumber, rainfallCounter;
uint8_t tankId, pumpId;
EepromHandler eeprom;

void setup() { 
#if defined(ARDUINO_ARCH_ESP8266)
  Serial.begin(115200);
#elif defined(ARDUINO_ARCH_ESP32)
  Serial.begin(115200);
  delay(500);
  #if defined(WIFI_LoRa_32_V2) || defined(WIFI_LoRa_32_V3)
    pinMode(Vext,OUTPUT);
    digitalWrite(Vext,LOW); // Turn on external power
  #endif
#elif defined(__AVR_ATmega328P__)
  Serial.begin(9600);     // Pro Mini FTDI Adaptor can't handle high baud rates
#elif defined(__ASR_Arduino__)
  Serial.begin(115200);
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); // Turn on external power
#endif
  delay(500);
  Serial.print(F("[setup] "));
  Serial.print(F("I2C EEPROM Read/Write Utility "));
  Serial.print(String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus));
  Serial.print(F(" : "));
#if defined(ARDUINO_ARCH_ESP8266)
  Serial.println("Generic ESP8266");
#elif defined(ARDUINO_ARCH_ESP32)
  #if defined(WIFI_LoRa_32_V2)
    Serial.println("Heltec WiFi LoRa 32 V2");
  #elif defined(WIFI_LoRa_32_V3)
    Serial.println("Heltec WiFi LoRa 32 V3 / Wireless Shell V3 / Wireless Stick Lite V3");
  #elif
    Serial.println("Generic ESP32");
  #endif
#elif defined(__AVR_ATmega328P__)
  Serial.println(F("AVR ATmega328P (Pro Mini)"));
#elif defined(__ASR_Arduino__)
  Serial.println(F("ASR CubeCell [Plus]"));
#endif

// Initialise the I2C Bus
// Note that, when operating at 3.3V, 32K and larger EEPROMs must not be clocked above 100kHz

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Wire.begin(SDA,SCL);
#elif defined(__AVR_ATmega328P__)
  Wire.begin();
#elif defined(__ASR_Arduino__)
  Wire.begin();
#endif

// Initialise the EEPROM constructor and work out what addressing to use

  eeprom.begin(&Wire);
  smartSerial = eeprom.setSmartSerial();
  
//  Let everything settle down before continuing

  delay(500);

  if (smartSerial) {
    Serial.println(F("[setup] Smart Serial Addressing (32K+ EEPROM)"));
  } else {
    Serial.println(F("[setup] Standard Serial Addressing (16K- EEPROM)"));
  }
  
  Serial.println(F("[setup] Retrieve data from EEPROM..."));
  printEepromContent();
  promptForInput();
}

void loop() {
  if ( Serial.available() > 0 ) {
    inputString = Serial.readString(); // read the command
    Serial.print(F("[loop] Input string: "));
    Serial.println(inputString);
    processInput( inputString );
    promptForInput();
  }
}

void printEepromContent() {
  gatewayMAC = eeprom.readUint32(EH_GATEWAY_MAC);
  Serial.print(F("   Gateway MAC (GM): 0x"));
  Serial.println(gatewayMAC,HEX);
  nodeMAC = eeprom.readUint32(EH_NODE_MAC);
  Serial.print(F("      Node MAC (NM): 0x"));
  Serial.println(nodeMAC,HEX);
  Serial.print(F("    Descriptor (DS): "));
  descriptor = eeprom.readBytes(EH_DESCRIPTOR);
  int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
  for (int i = 0; i < byteCount; i++) {
    Serial.print((char)descriptor[i]);
  }
  Serial.println();
  sequenceNumber = eeprom.readUint16(EH_SEQUENCE);
  Serial.print(F("    Sequence # (SN): "));
  Serial.println(sequenceNumber);
  rainfallCounter = eeprom.readUint16(EH_RAINFALL);
  Serial.print(F("  Rain Counter (RC): "));
  Serial.println(rainfallCounter);
  tankId = eeprom.readUint16(EH_TANKID);
  Serial.print(F("       Tank ID (TD): "));
  Serial.println(tankId);
  pumpId = eeprom.readUint16(EH_PUMPID);
  Serial.print(F("       Pump ID (PD): "));
  Serial.println(pumpId);
}

void promptForInput() {
  Serial.println();
  Serial.println(F("Enter  STandard or SMart to set Standard (16K-) or Smart (32K+) Addressing"));
  Serial.println(F("       <Return> to display current EEPROM dataset [GM/NM/DS/SN/RC]"));
  Serial.println(F("       GM or NM = <4-byte HEX MAC address> to set Gateway or Node MAC Address"));
  Serial.println(F("       DS = <text> to set Node Descriptor"));
  Serial.println(F("       SN, RC, TD or PD = <integer> to set Sequence #, Rain Counter, Tank ID or Pump ID respectively"));
  Serial.println(F("       HX for HEX dump of EEPROM contents"));
  Serial.println(F("       SC to scrub EEPROM contents (set bytes to 0xFF)"));
  Serial.println();
}

void processInput( String inputString ) {
  int i = 0;
  String upperString = inputString;
//  Serial.print("[processInput] String: ");
//  Serial.println(inputString);
  upperString.toUpperCase();
//  Serial.print("[processInput] Convert to Upper Case: ");
//  Serial.println(upperString);
  
  String character;
  int charVal;
  int stringLength = inputString.length();

  char inputBuf[50], upperBuf[50];
  inputString.toCharArray(inputBuf,50);
  upperString.toCharArray(upperBuf,50);
//  Serial.print("[processInput] Convert to Char array: ");
//  Serial.println(buf);
  
//  Serial.println("Skip leading white space...");
  i = 0;
  while ( isspace(upperBuf[i]) ) i++;
/*
  Serial.print("At position ");
  Serial.println(i);

  Serial.println("Extract identifier...");
  Serial.print("Substring: ");
  Serial.println(inputString.substring(i,i+2));
  Serial.println("Extract to Char array...");
*/
  upperString.substring(i,i+2).toCharArray(identifier,3);
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
  Serial.println(")");

  Serial.print("Extracted Identifier, index at : ");
  Serial.println(i);
*/
  i = i + 2;
  readOn = true;
  printAll = false;
  parameter = EH_NUM_TYPES;
  if ( strcmp(identifier,GM) == 0 ) {
//    Serial.println("Identifier is GM");
    parameter = EH_GATEWAY_MAC;
  } else if ( strcmp(identifier,NM) == 0 ) {
//    Serial.println("Identifier is NM");
    parameter = EH_NODE_MAC;
  } else if ( strcmp(identifier,DS) == 0 ) {
//    Serial.println("Identifier is DS");
    parameter = EH_DESCRIPTOR;
  } else if ( strcmp(identifier,SN) == 0 ) {
//    Serial.println("Identifier is SN");
    parameter = EH_SEQUENCE;
  } else if ( strcmp(identifier,RC) == 0 ) {
//    Serial.println("Identifier is RC");
    parameter = EH_RAINFALL;
  } else if ( strcmp(identifier,TD) == 0 ) {
//    Serial.println("Identifier is TD");
    parameter = EH_TANKID;
  } else if ( strcmp(identifier,PD) == 0 ) {
//    Serial.println("Identifier is PD");
    parameter = EH_PUMPID;
  } else if ( strcmp(identifier,HX) == 0 ) {
//    Serial.println("Identifier is HX");
/*
// - - - - - - - - - -
    Serial.println("[processInput] We're about to dump the EEPROM contents, but here's what we think is in there:");
    printEepromContent();
    eeprom.setDebug(true);
    Serial.println("[processInput] The Gateway MAC address again...");
    uint32_t echoMAC = eeprom.readUint32(EH_GATEWAY_MAC);
    Serial.print(F("   Gateway MAC (GM): 0x"));
    Serial.println(echoMAC,HEX);
    Serial.println("[processInput] Now let's try that manually...");
    eeprom.read(debugBuffer, 8, 0);
    eeprom.setDebug(false);
    Serial.println("[processInput] The first 8 bytes via direct read...");
    for (int i = 0; i < 8; i++) {
      Serial.print("0x");
      Serial.print(debugBuffer[i],HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    Serial.println();
    Serial.println("[processInput] Now the first 8 bytes via explicit dump...");
    eeprom.dump(0,7);
    Serial.println("[processInput] And the next 8 bytes via explicit dump...");
    eeprom.dump(8,15);
    Serial.println("[processInput] Now all 16 bytes via explicit dump...");
    eeprom.dump(0,15);
    Serial.println("[processInput] Now 32 bytes via explicit dump...");
    eeprom.dump(0,31);
    Serial.println("[processInput] Now 64 bytes via explicit dump...");
    eeprom.dump(0,63);
    Serial.println("[processInput] Now 128 bytes via explicit dump...");
    eeprom.dump(0,127);
    Serial.println("[processInput] The second 128 bytes via explicit dump...");
    eeprom.dump(128,255);
    Serial.println("[processInput] Now all 255 bytes via explicit dump...");
    eeprom.dump(0,255);
// - - - - - - - - - - */
    eeprom.dump();
    readOn = false;
  } else if ( strcmp(identifier,SC) == 0 ) {
//    Serial.println("Identifier is SC");
    eeprom.scrub();
    readOn = false;
  } else {
    readOn = false;
    if ( strcmp( identifier,standard ) == 0 ) {
      smartSerial = false;
      eeprom.setSmartSerial(smartSerial);
      Serial.println(F("Set Standard [1 byte] Addressing"));
    } else if ( strcmp( identifier,smart ) == 0 ) {
      smartSerial = true;      
      eeprom.setSmartSerial(smartSerial);
      Serial.println(F("Set Smart [2 byte] Addressing"));
    } else {
      if ( strcmp( identifier,returnChar ) == 0 ) {
        printAll = true;
      } else {
        Serial.print(F("Command <"));
        Serial.print(identifier);
        Serial.println(F("> not recognised"));        
      }
    }
  }

// Also need code to input HEX for MAC addresses or char for Descriptor
// Use isHexadecimalDigit(buf[i]) for HEX
// Use isPrintable(buf[i]) for legitimate Descriptor characters

  // Now see if a new value for the parameter has been entered

  if (readOn) {
//    Serial.println(F("Reading on for input data..."));
    switch (parameter) {
      case EH_GATEWAY_MAC:
      case EH_NODE_MAC: {
          // Come here if there's more to read      
        while ( !isHexadecimalDigit(inputBuf[i]) && i < stringLength ) i++;
        if ( i < stringLength ) {
//          Serial.print("Got a HEX digit at position ");
//          Serial.println(i);
          int j = i;
          int k = 0;
          char hexBuf[50];
          while ( isHexadecimalDigit(inputBuf[j]) && j < stringLength ) {
            hexBuf[k] = inputBuf[j];
            k++;
            j++;
          }
          hexBuf[k] = '\0';
//          Serial.print("Found HEX integer at postions ");
//          Serial.print(i);
//          Serial.print(" to ");
//          Serial.println(j-1);
//          Serial.print("hexBuf : ");
//          Serial.println(hexBuf);
  
          identifier32Value = (uint32_t)strtoul(hexBuf, 0, 16);
      //    Serial.print("Integer value: 0x");
      //    Serial.println(identifier32Value,HEX);
  
          Serial.print(F("[processInput] Write "));
          Serial.print(identifier);
          Serial.print(F(" = 0x"));
          Serial.print(identifier32Value,HEX);
          Serial.println(F(" to EEPROM..."));
          eeprom.writeUint32(parameter, identifier32Value);
        } else {
          // No more input so just return the current value
          switch (parameter) {
            case EH_GATEWAY_MAC: {
              Serial.print(F(" Gateway MAC: 0x"));
              break;
            }
            case EH_NODE_MAC: {
              Serial.print(F("    Node MAC: 0x"));
              break;
            }
          }
          Serial.println(eeprom.readUint32(parameter),HEX);
        }
        break;
      }
      case EH_DESCRIPTOR: {
        Serial.print(F("Starting at index point : "));
        Serial.println(i);
        // There must be one character, usually a space, between the identifier and the descriptor
        i++;
        while ( !isPrintable(inputBuf[i]) && i < stringLength ) i++;
        if ( i < stringLength ) {
//          Serial.print("Got a character at position ");
//          Serial.println(i);
          int j = i;
          while ( isPrintable(inputBuf[j]) && j < stringLength ) j++;
          
          // Serial.print("Found printable characters at postions ");
          // Serial.print(i);
          // Serial.print(" to ");
          // Serial.println(j-1);
  
          uint8_t bufferSubstring[50];
          int k = 0;
          for (int l = i; l < j; l++) {
            bufferSubstring[k] = inputBuf[l];
            k++;
          }
          int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
          for (int l = k; l < byteCount; l++) {
            bufferSubstring[l] = 0;
          }
          Serial.print(F("Write "));
          Serial.print(identifier);
          Serial.print(F(" = ["));
          for (int m = 0; m < k; m++) {
            Serial.print((char)bufferSubstring[m]);
          }
          Serial.print(F("] ("));
          Serial.print(k);
          Serial.println(F(" bytes) to EEPROM..."));
          eeprom.writeBytes(EH_DESCRIPTOR, bufferSubstring, byteCount);
        } else {
          // No more input so just return the current value
          Serial.print(F("  Descriptor: "));
          descriptor = eeprom.readBytes(EH_DESCRIPTOR);
          int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
          for (int n = 0; n < byteCount; n++) {
            Serial.print((char)descriptor[n]);
          }
          Serial.println();
        }
        break;
      }
      case EH_SEQUENCE:
      case EH_RAINFALL: {
        while ( !isdigit(inputBuf[i]) && i < stringLength ) i++;
        if ( i < stringLength ) {
      //    Serial.print("Got a digit at position ");
      //    Serial.println(i);
          int j = i;
          while ( isdigit(inputBuf[j]) && j < stringLength ) j++;
      //    Serial.print("Found integer at postions ");
      //    Serial.print(i);
      //    Serial.print(" to ");
      //    Serial.println(j-1);
  
          identifier16Value = inputString.substring(i,j).toInt();
      //    Serial.print("Integer value: ");
      //    Serial.println(identifier16Value);
  
          Serial.print(F("Write "));
          Serial.print(identifier);
          Serial.print(F(" = "));
          Serial.print(identifier16Value);
          Serial.println(F(" to EEPROM..."));
          eeprom.writeUint16(parameter, identifier16Value);
        } else {
          switch (parameter) {
            case EH_SEQUENCE: {
              Serial.print(F("  Sequence #: "));
              break;
            }
            case EH_RAINFALL: {
              Serial.print(F("Rain Counter: "));
              break;
            }
          }
          Serial.println(eeprom.readUint16(parameter));
        }
        break;
      }
      case EH_TANKID:
      case EH_PUMPID: {
        while ( !isdigit(inputBuf[i]) && i < stringLength ) i++;
        if ( i < stringLength ) {
      //    Serial.print("Got a digit at position ");
      //    Serial.println(i);
          int j = i;
          while ( isdigit(inputBuf[j]) && j < stringLength ) j++;
      //    Serial.print("Found integer at postions ");
      //    Serial.print(i);
      //    Serial.print(" to ");
      //    Serial.println(j-1);
  
          identifier8Value = inputString.substring(i,j).toInt();
      //    Serial.print("Integer value: ");
      //    Serial.println(identifier8Value);
  
          Serial.print(F("Write "));
          Serial.print(identifier);
          Serial.print(F(" = "));
          Serial.print(identifier8Value);
          Serial.println(F(" to EEPROM..."));
          eeprom.writeUint8(parameter, identifier8Value);
        } else {
          switch (parameter) {
            case EH_TANKID: {
              Serial.print(F("     Tank ID: "));
              break;
            }
            case EH_PUMPID: {
              Serial.print(F("     Pump ID: "));
              break;
            }
          }
          Serial.println(eeprom.readUint8(parameter));
        }
        break;
      }
      default: {
        
        break;
      }
    }
  }
    
  if ( printAll ) {
    if (smartSerial) {
      Serial.println(F("Smart Serial Addressing (32K+ EEPROM)"));
    } else {
      Serial.println(F("Standard Serial Addressing (16K- EEPROM)"));
    }
    printEepromContent();
  }
 }

 #if defined(__ASR_Arduino__)
bool isHexadecimalDigit( char character ) {
  bool result = false;
  if ((( character >= '0' ) && ( character <= '9')) || (( character >= 'A' ) && ( character <= 'F')) || (( character >= 'a' ) && ( character <= 'f'))) {
    result = true;
  }
  return result;
}

bool isPrintable( char character ) {
  bool result = false;
  if (( character >= ' ') && ( character <= '~' )) {
    result = true;
  }
  return result;
}
#endif
