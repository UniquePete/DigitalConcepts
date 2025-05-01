/* EEPROM Read Test
  
   Digital Concepts
   18 Apr 2024
   digitalconcepts.net.au
  
 */

#include <Arduino.h>
#include <Wire.h>               // I2C Bus
#include <EepromHandler.h>		  // EEPROM management
#include "CubeCellPlusPins.h"   // CubeCell Plus pin definitions

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {0,0,1};
String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

// Instantiate the objects we're going to need and declare their associated variables

EepromHandler eeprom;

uint8_t* descriptor;
uint16_t messageCounter = 0;
uint8_t tankID = 0;

uint16_t rainCounter = 0;
unsigned long previousRainInterrupt = 0;
unsigned long debounceInterval = 50;      // 50 mSec

// LowPower stuff

#define sleepTime 10000
static TimerEvent_t wakeUp;

typedef enum {
    LOWPOWER,
    TX
} States_t;

States_t state;

void onWake() {
  Serial.println("[onWake] Wake and enter TX state...");
  Serial.println();

  digitalWrite( Vext, LOW ); // Turn on power to EEPROM & sensors
  delay(100);               // Give everything a moment to settle down
  Wire.end();
  Wire.begin();
  eeprom.begin( &Wire );

  state = TX;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Initialise the EEPROM Handler
  
  pinMode( Vext, OUTPUT );
  digitalWrite( Vext, LOW ); // Activate Vext (we're really writing to Vext_Ctrl here)
  Wire.begin();
  eeprom.begin( &Wire );
  
  Serial.print("[setup] ");
  descriptor = eeprom.readBytes( EH_DESCRIPTOR );
  int byteCount = eeprom.getParameterByteCount( EH_DESCRIPTOR );
  for (int i = 0; i < byteCount; i++) {
    Serial.print((char)descriptor[i]);
  }
  Serial.println();
  Serial.println("[setup]        Sketch " + sketchRev);
  Serial.println("[setup] EepromHandler " + eeprom.softwareRevision());
  Serial.println();

  PINMODE_INPUT_PULLUP( rainCollectorPin );
  attachInterrupt( rainCollectorPin,incrementRainCount,FALLING );
  
  TimerInit( &wakeUp, onWake );
  TimerSetValue( &wakeUp, sleepTime );
  state = TX;
  
  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop() {
//  Serial.println("[loop] Enter Loop");
	switch ( state ) {
		case TX: {

      eeprom.setDebug( true );
/*
      rainCounter = 2738;
      Serial.print("[loop-TX] Write the Rain Counter: ");
      Serial.println( rainCounter );
      Serial.println();
 
      eeprom.writeUint16( EH_RAINFALL, rainCounter ); // Save the rain counter

      Serial.println();
      Serial.println("[loop-TX] Now read it back...");
      Serial.println();

      rainCounter = eeprom.readUint16( EH_RAINFALL );

      Serial.println();
      Serial.print("[loop-TX] Rain Counter read from index ");
      Serial.print( EH_RAINFALL );
      Serial.print(" : ");
      Serial.println( rainCounter );
*/
      Serial.println();
      Serial.println("[loop-TX] Now read the Sequence Number...");
      Serial.println();

      messageCounter = eeprom.readUint16( EH_SEQUENCE );
      
      Serial.println();
      Serial.print("[loop-TX] Sequence Number read from index ");
      Serial.print( EH_SEQUENCE );
      Serial.print(" : ");
      Serial.println( messageCounter );
      Serial.println();
      Serial.println("[loop-TX] Now read the Rain Counter...");
      Serial.println();

      rainCounter = eeprom.readUint16( EH_RAINFALL );

      Serial.println();
      Serial.print("[loop-TX] Rain Counter read from index ");
      Serial.print( EH_RAINFALL );
      Serial.print(" : ");
      Serial.println( rainCounter );
      Serial.println();

      Serial.print("[loop-TX] Increment and write Sequence Number : ");
      Serial.println( ++messageCounter );
      Serial.println();

      eeprom.writeUint16( EH_SEQUENCE, messageCounter ); // Save the message counter

      Serial.println();
      Serial.println("[loop-TX] Now read the Sequence Number back... ");
      Serial.println();

      messageCounter = eeprom.readUint16( EH_SEQUENCE );     

      Serial.println();
      Serial.print("[loop-TX] Sequence Number : ");
      Serial.println( messageCounter );
      Serial.println();

      Serial.println("[loop-TX] Read the Rain Counter... ");
      Serial.println();

      rainCounter = eeprom.readUint16( EH_RAINFALL );

      Serial.println();
      Serial.print("[loop-TX] Rain Counter : ");
      Serial.println( rainCounter );
      Serial.println();

      eeprom.setDebug( false );

      Serial.println(); 
      delay(100);
      state=LOWPOWER;      
	    break;
    }
		case LOWPOWER: {
//      Serial.println("[loop-LOWPOWER] Enter low power mode...");
      digitalWrite( Vext, HIGH );               // Turn off the external power supply before sleeping
      TimerStart( &wakeUp );
//      delay(1000);
//      state=TX;      
      lowPowerHandler();
      break;
    }
    default:
        break;
	}
}

void incrementRainCount() {
  unsigned long time = 0;
  time = millis();
  if (( time - previousRainInterrupt ) > debounceInterval ) {
//    Serial.println("[incrementRainCount] Increment rain counter...");
//    Serial.print("         was: ");
    digitalWrite(Vext, LOW);    // Make sure the external power supply is on
    Wire.begin();               // On with the show...
    rainCounter = eeprom.readUint16( EH_RAINFALL );
//    Serial.println( rainCounter );
  
    rainCounter++;
//    Serial.print("          is: ");
//    Serial.println( rainCounter );
    eeprom.writeUint16( EH_RAINFALL, rainCounter );
  }
  previousRainInterrupt = time;
}