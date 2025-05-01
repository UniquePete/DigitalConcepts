/*
  Interrupt Display test

  Function:
  Generate an interrupt when the interrupt button is pressed and trigger the activation of the on-board display.
  The pin on which the interrupt is configured can be set by PCB identifier or manually. Interrupts can be set
  to trigger on the RISING or FALLING edge, or simply on a CHANGE (both RISING and FALLING).

  1 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au
*/
#include <Wire.h>               // I2C Bus
#include <HT_SH1107Wire.h>      // CubeCell Plus display library
#include "CubeCellPlusPins.h"   // CubeCell Plus pin definitions

#define interruptPin GPIO11   // Interrupt button on 10068-BHCP PCB
#define displayTime 10000

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {0,0,1};
String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

#define sleepTime 60000
static TimerEvent_t wakeUp;

SH1107Wire  display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10); // addr, freq, sda, scl, resolution, rst

static TimerEvent_t displaySleep;
bool interruptFlag = false;
bool oledOff = true;

static TimerEvent_t watchDogTimer;
const int wdtInterval = 60000;  // Milliseconds
unsigned long wdtLimit = 180000;  // Milliseconds

typedef enum {
    LOWPOWER,
    TX
} States_t;

States_t state;

void onWake() {
  Serial.println(); 
  Serial.println("[onWake] Wake and set TX state...");

  digitalWrite( Vext, LOW );  // Turn on power to EEPROM & sensors
  delay(100);                 // Give everything a moment to settle down
  Wire.end();                 // The Wire instance seems to need to be reset before use...
  Wire.begin();

  state = TX;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] Interrupt test");

  PINMODE_INPUT_PULLUP(interruptPin);
  attachInterrupt( interruptPin, isr, FALLING );
/*
  pinMode( Vext, OUTPUT );
  digitalWrite( Vext, LOW ); // Activate Vext (we're really writing to Vext_Ctrl here)
  Wire.begin();
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.screenRotate(ANGLE_180_DEGREE);
  display.clear();
  display.setFont(ArialMT_Plain_16);
  String messageString = "Testing...";
  display.drawString(10, 20, messageString);
  display.display();
*/
  TimerInit( &wakeUp, onWake );
  TimerSetValue( &wakeUp, sleepTime );
  state = TX;
/*
  pinMode( wdtDone, OUTPUT );
  digitalWrite( wdtDone, LOW );      // TPL5010 reset
  TimerInit( &watchDogTimer, patTheDog );
  TimerSetValue( &watchDogTimer, wdtInterval );
  TimerStart( &watchDogTimer );

  TimerInit( &displaySleep, deactivateOLED );
  TimerSetValue( &displaySleep, displayTime );
*/
  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop() {
  if ( interruptFlag ) {
    Serial.println("[loop] Falling interrupt detected"); // Actually, the logic here seems to be inverted...
    interruptFlag = false;
  }
	switch ( state ) {
		case TX: {
      Serial.println("[loop-TX] Into TX mode...");
      state = LOWPOWER;
	    break;
    }
		case LOWPOWER: {
      digitalWrite( Vext, HIGH );               // Turn off the external power supply before sleeping
      TimerStart( &wakeUp );
      lowPowerHandler();
      break;
    }
    default:
      break;
	}
}

void isr() {
  interruptFlag = true;
  displaySoftRev();
}

void displaySoftRev() {
  /*
// Initialise the OLED display
  
  Serial.println("[setup] Initialise display...");
  pinMode(RST_OLED,OUTPUT);     // GPIO16
  digitalWrite(RST_OLED,LOW);   // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  display.init();
  display.clear();
//  myDisplay.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5,5,"Display Test");
  display.display();
  */
  String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);
  if ( oledOff ) {
    Serial.println();
    Serial.println("[displaySoftwareRevison] Turn on OLED display...");
    Serial.println();

    display.displayOn(); // Switch on display
    delay(10);
    oledOff = false;
  }
  Serial.println("[displaySoftwareRevison] Display software revision data...");
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.println("[displaySoftwareRevison] Gateway  " + sketchRev);
  display.drawString(5,6,"Gateway");
  display.drawString(60,6,sketchRev);
  Serial.println();
  display.display();

  TimerStart( &displaySleep );
}

void deactivateOLED() {
  Serial.println();
  Serial.println("[deactivateOLED] Timer expired... deactivating OLED");
  Serial.println();
  display.displayOff(); // Switch off display
  oledOff = true;
}

void patTheDog() {
  Serial.println("[patTheDog] Pat the dog...");
  digitalWrite(wdtDone,HIGH);
  digitalWrite(wdtDone,LOW);
  TimerStart( &watchDogTimer );
}