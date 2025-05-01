#include "Adafruit_BME280.h"  // BME280 class & methods
#include "Adafruit_Sensor.h"  // BME280 sensor support methods
#include "Wire.h"             // Need this to define second I2C bus

//#include "ProMiniPins.h"
//#include "ESP12Pins.h"
//#include "NodeMCUPins.h"
//#include "ESP32Pins.h"
//#include "WiFiLoRa32V1Pins.h"
#include "WiFiLoRa32V3Pins.h"
//#include "ESP32DevKitPins.h"

// BME280 Settings
const int I2C_BME_Address = 0x76;

// Define BME280 sensor
Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries

uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)

const int reportInterval = 5000;     // Report values every 10000 milliseconds (10 seconds)

int messageCounter = 0;

void setup() {
  pinMode(Builtin_LED,OUTPUT);     // Will use LED to signal packet transmission
  digitalWrite(Builtin_LED,LOW);  // Turn it on...

  pinMode(Vext_Ctrl,OUTPUT);     // Power supply to connected devices
  digitalWrite(Vext_Ctrl,LOW);  // Turn it on
  
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.println("[setup] I2C BME Test");

// Initialise the BME280 sensor
  Serial.println("[setup] Initialise BME280 sensor");
  Wire.begin(SDA,SCL);
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println("[setup] BME280 sensor initialisation complete");
  } else {
    Serial.println("[setup] Could not find a valid BME280 sensor, check wiring!");
//    while (true);
  }
  Serial.println();

  Serial.println("[setup] Set-up Complete");

  //  Let everything settle down before continuing
  delay(500);
}

void loop() {
  readSensor();
  Serial.println("[loop] Delay...");
  delay(reportInterval);
}

void readSensor() {
  temperature = (int) (10*bme280.readTemperature());
  pressure = (int) (bme280.readPressure() / 91.79F);
  humidity = (int) (bme280.readHumidity());

  Serial.println("");
  Serial.print("[readSensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readSensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readSensor] Humidity: ");
  Serial.println(humidity);
}
