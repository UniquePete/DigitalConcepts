#include "WSLPins.h"      // Wireless Stick Lite Pin Definitions
#include "LoRa.h"
#include "esp_sleep.h"
#include "Arduino.h"
#include "Adafruit_BME280.h"  // BME280 class & methods
#include "Adafruit_Sensor.h"  // BME280 sensor support methods
#include "Wire.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#include "CRC32.h"            // Checksum methods
#include "LangloLoRa.h"       // LoRa configuration parameters
#include "PacketHandler.h"    // Packet structures
#include "nodeS2Dconfig.h"     // Specific Node configuration parameters

// The counter needs to be stored in RTC memory to be preserved across deep sleep
RTC_DATA_ATTR int messageCounter = 0;

// Voltage divider
#define ACTIVATE_PIN GPIO32
#define VOLTAGE_PIN A4a_A0
const float VOLTAGE_FACTOR = 1.0120; // Calibrated voltage conversion factor
int pinLevel = 0;
int batteryVoltage = 0; // In mV

// Rainfall conversion factor (?)
const float LSB = 0.002761647; // per manual calibration
int inputTankLevel = 0;
int adc0 = 0;

// Sensor value variables
uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

uint16_t windBearing = 0;
uint8_t windSpeed = 0;

uint16_t inputRainfall = 0;

// BME280 Settings
#define SEALEVELPRESSURE_HPA (1013.25)
const int I2C_BME_Address = 0x76;

Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries

// Dallas Temperature Sensor
#define ONE_WIRE_BUS    A4a_A2

OneWire ds18b20(ONE_WIRE_BUS);
DallasTemperature sensors(&ds18b20);
float ds18_temperature = 0;

PacketHandler packet;

void setup() {
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" Wireless Stick Lite Handler");
  Serial.println();

  LoRa.setPins(CS,RST,DIO0);
  Serial.println("[setup] Activate LoRa Sender");
  if (!LoRa.begin(Frequency)) {
    Serial.println("[setup] Starting LoRa failed!");
    while (true);
  }
 
  Serial.print("[setup] LoRa Frequency: ");
  Serial.println(Frequency);

  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  Serial.println("[setup] LoRa initialisation complete");
  Serial.println();

//  Initialise the battery monitor
  pinMode(ACTIVATE_PIN, OUTPUT);
  digitalWrite(ACTIVATE_PIN, LOW);
  
  pinMode(VOLTAGE_PIN, INPUT);
  
// initialise the packet object
  
  packet.begin(gatewayMAC,myMAC,messageCounter);

  Serial.print( "[setup] Sequence #: " );
  Serial.println(messageCounter);

  Serial.println( "[setup] Read battery voltage" );
  digitalWrite(ACTIVATE_PIN, HIGH);
  Serial.println( "[setup] Activate HIGH" );
  delay(1000);
  pinLevel = analogRead(VOLTAGE_PIN);
  digitalWrite(ACTIVATE_PIN, LOW);
  Serial.print( "[setup] Pin level: " );
  Serial.println( pinLevel );
  Serial.print( "[setup] Battery Voltage: " );
  batteryVoltage = (int) pinLevel * VOLTAGE_FACTOR;
  Serial.println( (float) batteryVoltage/1000 );
  Serial.println();
  
  packet.setPacketType(VOLTAGE);
  packet.setVoltage(batteryVoltage);

  Serial.println("[setup] Send Message");
  sendMessage();

// Initialise the DS18B20 sensor
  Serial.println("[setup] Initialise DS18B20 sensor");
  sensors.begin();
  Serial.println("[setup] DS18B20 sensor initialisation complete");
  Serial.println("");

  delay(100);  // Let things settle before reading
  
  packet.setPacketType(TEMPERATURE);
  readDS18B20();    
  Serial.println("[setup] Send Message");  
  sendMessage();
  
  delay(100);  // Give the gateway a tick to process that before sending the next message

// Initialise the BME280 sensor
  Serial.println("[setup] Initialise BME280 sensor");
//  bmeI2C.begin(SDA, SCL, 400000);
//  if (bme280.begin(I2C_BME_Address, &bmeI2C)) {
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println("[setup] BME280 sensor initialisation complete");
  } else {
    Serial.println("[setup] Could not find a valid BME280 sensor, check wiring!");
//    while (true);
  }
  
  packet.setPacketType(WEATHER);
  readSensor();    
  Serial.println("[setup] Send Message");  
  sendMessage();
  delay(100);  // Give the gateway a tick to process that before sending the next message
 
// Read the tank level from ADC1_0 (Pin 36)
//  adc0 = analogRead(36);

//  inputTankLevel = adc0 * LSB;
  inputTankLevel = 85; // Dummy value (as a percentage) just so that there's someoething mroe than 0 to send

//  Serial.print("[Loop] Sending packet");
  
  packet.setPacketType(TANK);
  packet.setTankLevel(inputTankLevel);
  Serial.println("[setup] Set Tank Level");  
  Serial.println("[setup] Send Message");  
  sendMessage();
  messageCounter++;                 // Increment message count
  delay(100);  // Give us a moment to get the last messages output to the serial monitor before sleeping

  Serial.println("[setup] Off to sleep now...");  

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
//  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
//  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
//  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
//  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();

}

void loop() {
// There's nothing here because the processor and memory are effectively reset after coming out of deep sleep
}

void readDS18B20() {
  Serial.println("[readDS18B20] Check how many DS18B20 sensors we have...");
  byte i;
  byte addr[8];
  
  Serial.println("[readDS18B20] Searching for addresses...");
  while (ds18b20.search(addr)) {
    Serial.print(" ROM =");
    for (i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i] < 16 ? "0" : "");
      Serial.print(addr[i], HEX);
    }
    Serial.println();
  }
  Serial.println("[readDS18B20] No more addresses");
  Serial.println();
  ds18b20.reset_search();
  delay(250);

  Serial.println("[readDS18B20] Read the sensors...");
  sensors.requestTemperatures();  
  ds18_temperature = sensors.getTempCByIndex(0);       // [C    0.1]
  Serial.print("[readDS18B20] Sensor 0 Temperature: ");
  Serial.println( ds18_temperature, 1);
  temperature = (int) 10*ds18_temperature;
  Serial.print("[readDS18B20] Sensor 0 Message Value: ");
  Serial.println(temperature);
  packet.setTemperature(temperature);
//  ds18_temperature = sensors.getTempCByIndex(1);       // [C    0.1]
//  Serial.print("[readDS18B20] Sensor 1 Temperature: ");
//  Serial.println( ds18_temperature, 1);
}

void readSensor() {
  Serial.println("[readSensor] Read BME sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/ 
  temperature = (int) (10*bme280.readTemperature());
  pressure = (int) (bme280.readPressure() / 91.79F);
  humidity = (int) bme280.readHumidity();

  temperature = -1270;
  
  packet.setTemperature(temperature);
  packet.setPressure(pressure);
  packet.setHumidity(humidity);

  Serial.print("[readSensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readSensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readSensor] Humidity: ");
  Serial.println(humidity);

/*
 * When we've actually got a sensor to read, this variable should be set to the value
 * read from the sensor
 */
//  inputRainfall = (int) adc0 * LSB;
  inputRainfall = 255; // Dummy value (in 1/10 mm) just so taht there's somethign other than 0 to report

  packet.setRainfall(inputRainfall);

  Serial.print("[readSensor] Rainfall: ");
  Serial.println((float) inputRainfall/10, 1);

/*
 * When we've actually got a sensor to read, these variables should be set to the values
 * read from the sensor(s)
 */
  windBearing = 310;
  windSpeed = 15;

  packet.setWindBearing(windBearing);
  packet.setWindSpeed(windSpeed);

  Serial.print("[readSensor] Wind Bearing: ");
  Serial.println(windBearing);
  Serial.print("[readSensor] Wind Speed: ");
  Serial.println(windSpeed);

//  batteryVoltage = getBatteryVoltage();
//  Serial.print("[readSensor] Battery Voltage: ");
//  Serial.print( getBatteryVoltage() );
//  Serial.println(" mV");

}

void sendMessage() {

//  Serial.println("[sendMessage] Outgoing packet: ");
  digitalWrite(Builtin_LED, LOW);  // Turn on the LED to signal sending
  LoRa.beginPacket();             // start packet
  LoRa.write((uint8_t *)packet.byteStream(), packet.packetByteCount());    // Load the output buffer
 /* 
  int totalByteCount = packet.packetByteCount();
  Serial.print("[sendMessage] Byte Count: ");
  Serial.println( totalByteCount );
  for (int i = 0; i < totalByteCount; i++) {
    LoRa.write(outgoingPacket.packetByte[i]);
    Serial.printf( "%02X", outgoingPacket.packetByte[i]);
    if ((i % 4) == 3) {
      Serial.println("    ");
    } else {
      Serial.print("  ");
    }
  }
  if ((totalByteCount % 4) != 0) Serial.println();
*/
  Serial.println("[sendMessage] Finished Packet");
  LoRa.endPacket();             // Finish packet and send it
  Serial.println("[sendMessage] Packet Sent");
  digitalWrite(Builtin_LED, HIGH);  // Done
  Serial.println();
}
