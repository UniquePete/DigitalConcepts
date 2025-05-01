/* 
 *  Just a simple switch that is operated through the Serial Monitor
 *  
 * 10 Sep 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include <HT_SH1107Wire.h>     // CubeCell Plus display library

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#elif defined(__AVR_ATmega328P__)
#elif defined(__ASR_Arduino__)
  #include <AT24C32N.h>
#endif

//#include "ProMiniPins.h"
//#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
//#include "WiFiLoRa32V2Pins.h"
//#include "ESP32DevKitPins.h"
//#include "CubeCellPins.h"
#include "CubeCellPlusPins.h"

/* The following are the commends that will be recognised */
const char ON[]         = "ON";
const char OF[]         = "OF";
const char returnChar[] = "";   // Carriage Return - Display current values

String inputString;
char identifier[3];
int identifierValue;

#define switchPin A4a_A3

// The CubeCell Plus Display object
SH1107Wire  display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr, freq, sda, scl, resolution, rst

void setup() { 
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(50);
  delay(500);
  Serial.println();
  Serial.print("[setup] ");
  Serial.print(" Relay switch test Utility – ");
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  Serial.println("ESP8266/ESP32");
#elif defined(__AVR_ATmega328P__)
  Serial.println("ATmega328P (Pro Mini)");
#elif defined(__ASR_Arduino__)
  Serial.println("CubeCell [Plus]");
#endif
  Serial.println();

  pinMode( switchPin, OUTPUT );
  
  display.init();
  display.screenRotate(ANGLE_180_DEGREE);
  display.setFont(ArialMT_Plain_10);
  display.drawString(10, 5, "Relay Switch");
  display.display();
  
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
  Serial.println("Enter  ON or OF");
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
  display.clear();
  display.setFont(ArialMT_Plain_16);

  if ( strcmp(identifier,ON) == 0 ) {
    Serial.println("Switch is ON");
    display.drawString(10, 20, "Relay: ON");
    digitalWrite( switchPin, HIGH );
  } else if ( strcmp(identifier,OF) == 0 ) {
    Serial.println("Switch is OFF");
    display.drawString(10, 20, "Relay: OFF");
    digitalWrite( switchPin, LOW );
  }

  display.display();
}
