/* LoRa Sender Node
 *
 * Function:
 * Assemble packets using the PacketHandler and forward data to the nominated Gateway Node
 * 
 * Description:
 * 1. Uses packet structures defined in the PacketHandler library
 * 2. Output generated for Serial Monitor and/or OLED diaplay
 *
 * 7 Oct 2022 v1.0 Code sample for ESP32-WROOM-32 module with OLED display
 * 
 * 7 Oct 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include "esp_sleep.h"

#include "SPI.h"              // SPI Interface for RFM95
#include "LoRa.h"             // LoRa class & methods
#include "CRC32.h"            // Checksum methods
#include "Adafruit_BME280.h"  // BME280 class & methods
#include "Adafruit_Sensor.h"  // BME280 sensor support methods
#include "Wire.h"             // Need this to define the I2C bus

#include "LangloLoRa.h"       // LoRa configuration parameters
#include "PacketHandler.h"    // Packet structures
#include "ESP32Pins.h"        // ESP32 Pin Definitions
#include "nodeB4Dconfig.h"    // Node-specific configuration parameters

#define batteryMonitor    A4a_A0
#define wakePin           A4a_A1
#define donePin           A4a_A3

// Deep sleep stuff
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60       /* Time ESP32 will go to sleep (in seconds) */

// The counter needs to be stored in RTC memory to be preserved across deep sleep
// Not actually required when we're using the [external] EEPROM
RTC_DATA_ATTR int messageCounter = 0;

union EEPROMPayload {
  uint8_t payloadByte[2];
  uint16_t sequenceNumber;
};

const int I2C_EEPROM_Address = 0x50;
EEPROMPayload EEPROMWriteData, EEPROMReadData;

const int I2C_BME_Address = 0x76;

// Instantiate the packet object

PacketHandler packet;

uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

// Define BME280 sensor
Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)
uint16_t inputRainfall = 0;

uint16_t windBearing = 0;
uint16_t windSpeed = 0;

// ADC
static const float voltageFactor = (3.2/4096)*(127.0/100)*(4.20/4.20); // (max. ADC voltage/ADC range) * voltage divider factor * calibration factor
int inputValue = 0;
uint16_t batteryVoltage = 0;

void setup() {
  pinMode(Builtin_LED,OUTPUT);     // Will use LED to signal packet transmission
  digitalWrite(Builtin_LED, LOW);  // Turn it off to begin with
  pinMode(donePin,OUTPUT);         // Used to activate timer and turn off regulator
  pinMode(wakePin,OUTPUT);         // Used to activate battery monitor
 
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" I2C BME280/LoRa Sender");
  Serial.println();

//  Initialise the RFM95 radio
  SPI.begin(SCK,MISO,MOSI,NSS);
  LoRa.setPins(NSS,RST,DIO0);
  Serial.println("[setup] Activate LoRa Sender");
//  if (!LoRa.begin(BAND, PABOOST)) {
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
//  LoRa.dumpRegisters(Serial);

// Initialise the BME280 sensor
  Serial.println("[setup] Initialise BME280 sensor");
  Wire.begin(SDA,SCL);
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println("[setup] BME280 sensor initialisation complete");
  } else {
    Serial.println("[setup] Could not find a valid BME280 sensor, check wiring!");
//    while (true);  // Include this statement if we want to stop if the sensor is not found
  }
  
// Initialise EEPROM access

  Wire.begin();
  Wire.setClock(400000);

  Serial.println("[setup] Retrieve Sequence # from EEPROM...");
  EEPROMReadData.payloadByte[0] = readEEPROMData(0);
  EEPROMReadData.payloadByte[1] = readEEPROMData(1);
  messageCounter = EEPROMReadData.sequenceNumber;

// initialise the packet object
  
  packet.begin(gatewayMAC,myMAC,messageCounter);

//  Initialise the data packet with (DesitnationMAC, SourceMAC, SequanceNumber, PacketType)
//  PacketHandler class method should set ByteCount automatically according to PacketType

  Serial.println("[setup] Set-up Complete");
  Serial.println(""); 
  Serial.print("[setup] Sequence #: ");
  Serial.println(messageCounter); 

  //  Let everything settle down before continuing
  delay(100);

  // Read the battery voltage and send the status message off to the gateway
  packet.setPacketType(VOLTAGE);
  readBatteryVoltage();  
  Serial.println("[loop] Send Message");
  sendMessage();
  delay(100);

  packet.setPacketType(WEATHER);
  readBMESensor();
  Serial.println("[setup] Send Message");
  sendMessage();
  Serial.println("[setup] Increment Message Counter");
  messageCounter++;                 // Increment message count

//  Store Sequence # in EEPROM

  EEPROMWriteData.sequenceNumber = messageCounter;
  writeEEPROMData(0);

// Now, sleep the peripherals

  Serial.println("[setup] Turn off the battery level voltage divider...");
  digitalWrite(wakePin,LOW);

  Serial.println("[setup] Shut down the BME280...");
  BME280_Sleep(0x76);

  Serial.println("[setup] Shut down the RFM95W...");
  /*
  pinMode(NSS,OUTPUT);
  //Set NSS pin LOW to start communication
  digitalWrite(NSS,LOW);
  //Send Addres with MSB 1 to make it a write command
  SPI.transfer(0x01 | 0x80);
  //Send Data
  SPI.transfer(0x00);
  //Set NSS pin HIGH to end communication
  digitalWrite(NSS,HIGH);
  */
  LoRa.sleep();
  
  Serial.println("[setup] Now, off to sleep...");
  Serial.println(""); 

  digitalWrite(donePin,HIGH);

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

void readBatteryVoltage() {
  static const float voltageFactor = (3.2/4096)*(133.0/100)*(4.14/4.20); // (max. ADC voltage/ADC range) * voltage divider factor * calibration factor
  uint16_t inputValue = 0, peakValue = 0;
  Serial.println("[readBatteryLevel] Read ADC...");
  
  digitalWrite(wakePin,HIGH);
  delay(900);
  for (int i = 0; i < 10; i++) {
    /*
     * The ESP32 seems not to have a particularly good ADC, so we take 10 readings and choose the peak value
     * (becasue multiple values seem typically to follow a sine wave and we currently calibrate to the peak value)
     */
    delay(10);
    // and read ADC (Note the actual values of the voltage divider resistors in monitoring circuit)
    inputValue = analogRead(batteryMonitor); 
//    Serial.print(String(inputValue) + " = ");
//    Serial.println(String(inputValue*voltageFactor) + "V");
    if (inputValue > peakValue) peakValue = inputValue;
  }
  Serial.print(String(peakValue) + " = ");
  Serial.println(String(peakValue*voltageFactor) + "V");
  digitalWrite(wakePin,LOW);
  digitalWrite(A4a_A1,LOW);
  
  packet.setVoltage((int)(peakValue * voltageFactor * 1000));
}

void writeEEPROMData(long eeAddress)
{

  Wire.beginTransmission(I2C_EEPROM_Address);

//  Wire.write((int)(eeAddress >> 8)); // MSB
//  Wire.write((int)(eeAddress & 0xFF)); // LSB
  Wire.write( 0 );

  //Write bytes to EEPROM
  for (byte x = 0 ; x < 2 ; x++) {
    Wire.write(EEPROMWriteData.payloadByte[x]); //Write the data
//    Serial.print( "[writeEEPROMData] Byte " );
//    Serial.print( x );
//    Serial.print( ": " );
//    Serial.println( EEPROMWriteData.payloadByte[x] );
  }

  Wire.endTransmission(); //Send stop condition
}

byte readEEPROMData(long eeaddress)
{
  Wire.beginTransmission(I2C_EEPROM_Address);

//  Wire.write((int)(eeaddress >> 8)); // MSB
//  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write( eeaddress );

  Wire.endTransmission();

  Wire.requestFrom(I2C_EEPROM_Address, 1);
  delay(1);
  
  byte rdata = 0xFF;
  if (Wire.available()){
    rdata = Wire.read();
//    Serial.print( "[readEEPROMData] Byte: " );
//    Serial.println( rdata );
  } else {
    Serial.println( "[readEEPROMData] Nothing to read..." );
  }
  return rdata;
}

void BME280_Sleep(int deviceAddress) {
  Wire.beginTransmission(deviceAddress);
  Wire.write((uint8_t) 0xF4);
  Wire.write((uint8_t) 0b00000000);
  Wire.endTransmission();
}

void readBMESensor() {
  temperature = (int) (10*bme280.readTemperature());
  pressure = (int) (bme280.readPressure() / 91.79F);
  humidity = (int) (bme280.readHumidity());

  packet.setTemperature(temperature);
  packet.setPressure(pressure);
  packet.setHumidity(humidity);

  Serial.print("[readBMESensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readBMESensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readBMESensor] Humidity: ");
  Serial.println(humidity);

/*
 * When we've actually got a sensor to read, this variable should be set to the value
 * read from the sensor
 */
//  inputRainfall = (int) adc0 * LSB;
  inputRainfall = 155; // Dummy value (in 1/10 mm) just so taht there's somethign other than 0 to report

  packet.setRainfall(inputRainfall);
  
  Serial.print("[readBMESensor] Rainfall: ");
  Serial.println((float) inputRainfall/10, 1);

/*
 * When we've actually got a sensor to read, these variables should be set to the values
 * read from the sensor(s)
 */
  windBearing = 270;
  windSpeed = 10;

  packet.setWindBearing(windBearing);
  packet.setWindSpeed(windSpeed);
  
  Serial.print("[readBMESensor] Wind Bearing: ");
  Serial.println(windBearing);
  Serial.print("[readBMESensor] Wind Speed: ");
  Serial.println(windSpeed);
}

void sendMessage() {

// If the host is little endian, we need to reverse the byte order before sending

//  Serial.println("[sendMessage] Outgoing packet: ");
  digitalWrite(Builtin_LED, HIGH);  // Turn on the LED to signal sending
/*  
// Check that no one else is transmitting first
  while ((bool) LoRa.available()) {
    Serial.println("[sendMessage] Busy...");
// It would take 212 ms to transmit a [max size] 255 byte packet at 9600 bps
    delay(random(200));
  }
*/  
  LoRa.beginPacket();         // start packet
  LoRa.write(packet.byteStream(), packet.packetByteCount());    // Load the output buffer
/*  
  int totalByteCount = packet.packetByteCount();
  Serial.print("[sendMessage] Byte Count: ");
  Serial.println( totalByteCount );
  for (int i = 0; i < totalByteCount; i++) {
    LoRa.write(packet.byte(i));
    Serial.printf( "%02X", packet.byte(i));
    if ((i % 4) == 3) {
      Serial.println("    ");
    } else {
      Serial.print("  ");
    }
  }
  if ((totalByteCount % 4) != 0) Serial.println();
*/
//  Serial.println("[sendMessage] Finished Packet");
//  LoRa.dumpRegisters(Serial);
  LoRa.endPacket();                 // Finish packet and send it
  Serial.println("[sendMessage] Packet Sent");
  digitalWrite(Builtin_LED, LOW);   // Done
  Serial.println();
}
