/* Basic BME280 test for CubeCell
 *
 * Function:
 * Read BME280 sensor and report response
 * 
 * Rev 1.0  23 Nov 2022
 * Digital Concepts
 * digitalconcepts.net.au
 *
 */

#include <Seeed_BME280.h>
#include <Wire.h>

//#include "CubeCellPins.h"     	// CubeCell pin definitions
#include "CubeCellPlusPins.h"	// CubeCell Plus pin definitions

const uint8_t softwareRevMajor = 1;
const uint8_t softwareRevMinor = 0;

// Node name, just for the heck of it
const char* myName = "Default";

// Sensor value variables
uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)

const int reportInterval = 10000;     // Report values every 10000 milliseconds (10 seconds)

uint16_t messageCounter = 0;

// BME280 Settings (2 possible BME280 I2C addresses)
 const int I2C_BME_Address_1 = 0x76;
 const int I2C_BME_Address_2 = 0x77;

// Define BME280 sensor
BME280 bme280;					// Default I2C_BME_Address = 0x76

void setup() {
  boardInitMcu( );
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); //SET POWER
  Serial.begin(115200);
  delay(200);                         //Let things settle down..

  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" CubeCell BME280 Test");
  Serial.println("[setup] BME Sketch Rev: " + String(softwareRevMajor) + "." + String(softwareRevMinor));
  Serial.println();

  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop() {
  readBMESensor();
  Serial.println("[loop] Delay...");
  delay(reportInterval);
}

void readBMESensor() {
  bool OKtoGO = false;
  
  Serial.println("[readSensor] Read sensor...");

  digitalWrite(Vext, LOW);    // Turn on the external power supply
  delay(50);                  // Give everything a moment to settle down
  Wire.begin();               // On with the show...
  
  Serial.println("[readBMESensor] Check possible BME280 sensor addresses...");
  Serial.print("[readBMESensor] Try 0x");
  Serial.print(I2C_BME_Address_1,HEX);
  Serial.println("...");
  if (bme280.init(I2C_BME_Address_1)) {
    Serial.println("[readBMESensor] Sensor found");
    OKtoGO = true;
  } else {
    Serial.print("[readBMESensor] Try 0x");
    Serial.print(I2C_BME_Address_2,HEX);
    Serial.println("...");
    if (bme280.init(I2C_BME_Address_2)) {
      Serial.println("[readBMESensor] Sensor found");
      OKtoGO = true;
    } else {
    Serial.println( "[readBMESensor] Cannot find BME sensor" );
    }
  }
  
  if (OKtoGO) {
    Serial.println( "[readBMESensor] BME sensor initialisation complete" );

	  delay(100); // The BME280 needs a moment to get itself together... (50ms is too little time)

	  temperature = (int) (10*bme280.getTemperature());
	  pressure = (int) (bme280.getPressure() / 91.79F);
	  humidity = (int) (bme280.getHumidity());

		Serial.println();
		Serial.print("[readBMESensor] Temperature: ");
		Serial.println((float) temperature/10, 1);
		Serial.print("[readBMESensor] Pressure: ");
		Serial.println(pressure);
		Serial.print("[readBMESensor] Humidity: ");
		Serial.println(humidity);
  }

  Wire.end();
  digitalWrite(Vext, HIGH);    // Turn off the external power supply

}
