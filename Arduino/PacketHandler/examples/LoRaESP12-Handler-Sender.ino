/*
Peter Harrison
Digital Concepts
1 Oct 2021

Mux configuration test for A4a-NodeMCU processor board
*/
#include "SPI.h"                // SPI Interface for RFM95
#include "LoRa.h"               // LoRa class & methods
#include "CRC32.h"              // Checksum methods
#include "Adafruit_BME280.h"    // BME280 class & methods
#include "Adafruit_Sensor.h"    // BME280 sensor support methods

#include "LangloLoRa.h"         // LoRa configuration parameters
#include "PacketHandler.h"      // Packet structures
#include "ESP12Pins.h"          // ESP12 Pin Definitions
#include "nodeF5Dconfig.h"       // Node-specific configuration parameters

#define zInput          A4a_A0        // Connect mux common (Z) to A0 (analog input)
#define wakeSignal      A4a_A1        // GPIO16
#define muxSelect       A4a_A2        // GPIO10 (Just one pin because we're only using two ports)
#define muxY0           HIGH          // A4a Sandwich A0 (Voltage Divider)
#define muxY7           LOW           // A4a Sandwich A2 (TMP35/36)

// Packet structure
PacketHandler packet;

// EEPROM
union EEPROMPayload {
  uint8_t payloadByte[2];
  uint16_t sequenceNumber;
};
const int I2C_EEPROM_Address = 0x50;
EEPROMPayload EEPROMWriteData, EEPROMReadData;

// BME280
const int I2C_BME_Address = 0x76;

// Define BME280 sensor
Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries
bool bme280Present = false;

uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)
uint16_t inputRainfall = 0;

uint16_t windDirection = 0;
uint8_t windSpeed = 0;

// ADC
int inputValue = 0;

// Voltage divider
//const float voltageFactor = 4.026/1024;  // Measuring voltage across 22k/100k voltage divider (actually up to ~4.2V)
const float voltageFactor = 3.9/930;
uint16_t batteryVoltage = 0;

// TMP36
const float calibrationOffset = -9.9;

// General
int messageCounter = 0;
int reportInterval = 60000;  // In ms

void setup() 
{
  Serial.begin(115200); // Initialize the serial port
  delay(500);
  
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" ESP-12F EEPROM/BME/Mux/LoRa Sensor");
  Serial.println();

  pinMode(wakeSignal, OUTPUT);
  digitalWrite(wakeSignal, HIGH);

// Set up the necessary mux pins
// Port Select
  pinMode(muxSelect, OUTPUT);
  digitalWrite(muxSelect, HIGH);
  // Input (ADC)
  pinMode(zInput, INPUT);

//  Module initialisation
  initialiseLoRa();
  
  Wire.begin(SDA, SCL);         // Set the alternate I2C pins
  Wire.setClock(400000);
  initialiseBME280();
  initialiseEEPROM();

  packet.begin(gatewayMAC,myMAC);

  Serial.println("[setup] Set-up Complete");
  Serial.println();
  
//  Let everything settle down before continuing
  delay(500);
}

void loop() 
{
  Serial.println();
  Serial.println("[loop] Mux Ports:");
  
  packet.setSequenceNumber((uint16_t)messageCounter);
  packet.setPacketType(VOLTAGE);
  
  Serial.print("[loop] Select LOW (Y7) : ");
  digitalWrite(muxSelect, muxY7);
  digitalWrite(wakeSignal, HIGH);
  readBatteryVoltage();
  digitalWrite(wakeSignal, LOW);
  Serial.println("[loop] Send Message");
  sendMessage();
  delay(100);
  
  Serial.print("[loop] Select HIGH (Y0) : ");
  digitalWrite(muxSelect, muxY0);
  
  // Always leave GPIO10 HIGH to minimise opportunity for conflict with SPI usage
  // Not really sure if this makes any difference...
  
  packet.setPacketType(TEMPERATURE);
  readTMP36();
  Serial.println("[loop] Send Message");
  sendMessage();
  delay(100);

  if ( bme280Present ) {
    packet.setPacketType(ATMOSPHERE);
    readBME280();
    Serial.println("[loop] Send Message");
    sendMessage();
    delay(100);
  }

  messageCounter++;                 // Increment message count
  EEPROMWriteData.sequenceNumber = messageCounter;
  writeEEPROMData(0);               // Store Sequence # in EEPROM

  Serial.println();
  delay(reportInterval);
}

void initialiseLoRa() {  
//  SPI.begin(SCK,MISO,MOSI,NSS);
  SPI.begin();                      // Default is SPI1 (HSPI) with SCK (14), MISO (12) & MOSI (13)
//  LoRa.setPins(NSS,RST,DIO0);
  LoRa.setPins(NSS,RST,DIO0);            // No IRQ (DIO0) configured at this time - Not sure why...
  Serial.println("[initialiseLoRa] Activate LoRa Sender");
//  if (!LoRa.begin(BAND, PABOOST)) {
  if (!LoRa.begin(BAND)) {          // No PABOOST in teh ESP8266 impleentation it seems
    Serial.println("[initialiseLoRa] Starting LoRa failed!");
    while (true) {
      yield();                      // This just stops the ESP82665 from getting itself knotted
    }
  }

  Serial.print("[initialiseLoRa] LoRa Frequency: ");
  Serial.println(BAND);
  
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  
  Serial.println("[initialiseLoRa] LoRa initialisation complete");
  Serial.println();
}

void readTMP36() {
  inputValue = analogRead(zInput); // Read mux common (Z)
  Serial.print(inputValue);

  float voltage = inputValue * 3.3;
  voltage /= 1024.0; 
 
// Print out the voltage
//  Serial.print("[loop] Voltage: "); Serial.print(voltage); Serial.println(" V");
 
// Now print out the temperature
//  float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree with 500 mV offset
                                                //to degrees ((voltage - 500mV) x 100)
  temperature = (int) (((voltage - 0.5) * 100 + calibrationOffset) * 10); // °C x 10
  packet.setTemperature(temperature);
  
//  Serial.print("[loop] TempC: ");
  Serial.print(" = ");
//  Serial.print(temperature);
//  Serial.print(" = ");
  Serial.println(String(((float)temperature/10),1) + "°C");
}

void readBatteryVoltage() {
  inputValue = analogRead(zInput); // and read Z
  Serial.print(String(inputValue) + " = ");
  Serial.println(String(inputValue*voltageFactor) + "V");

  batteryVoltage = (int) (inputValue * voltageFactor * 1000);
  packet.setVoltage(batteryVoltage);
}

void initialiseBME280() {
  Serial.println("[initialiseBME280] Begin sensor initialisation...");
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println("[initialiseBME280] Sensor initialised");
    bme280Present = true;
  } else {
    Serial.println("[initialiseBME280] Could not find a valid BME280 sensor, check wiring or Builtin LED conflict!");
//    while (true){
//      yield();                      // Apparently, this just stops the ESP82665 from getting itself knotted
//    }
  }
  Serial.println();
}

void readBME280() {

  temperature = (int) (10*bme280.readTemperature());
  pressure = (int) (bme280.readPressure() / 91.79F);
  humidity = (int) (bme280.readHumidity());

  packet.setTemperature(temperature);
  packet.setPressure(pressure);
  packet.setHumidity(humidity);

  Serial.println("");
  Serial.print("[readBME280] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readBME280] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readBME280] Humidity: ");
  Serial.println(humidity);
}

void initialiseEEPROM() {
  Serial.println("[initialiseEEPROM] Retrieve Sequence # from EEPROM...");
  EEPROMReadData.payloadByte[0] = readEEPROMData(0);
  EEPROMReadData.payloadByte[1] = readEEPROMData(1);
  messageCounter = EEPROMReadData.sequenceNumber;
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

void sendMessage() {

// If the host is little endian, we need to reverse the byte order before sending

  Serial.println("[sendMessage] Outgoing packet: ");
  digitalWrite(Builtin_LED, LOW);  // Turn on the LED to signal sending
/*  
// Check that no one else is transmitting first
  while ((bool) LoRa.available()) {
    Serial.println("[sendMessage] Busy...");
// It would take 212 ms to transmit a [max size] 255 byte packet at 9600 bps
    delay(random(200));
  }
*/  
  LoRa.beginPacket();             // start packet
  LoRa.write((uint8_t *)packet.byteStream(), packet.packetByteCount());    // Load the output buffer

/*  
  int totalByteCount = LGO_HeaderSize + outgoingPacket.content.byteCount;
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

  Serial.println("[sendMessage] Finished Packet");
  */
  LoRa.endPacket();             // Finish packet and send it
  Serial.println("[sendMessage] Packet Sent");
  digitalWrite(Builtin_LED, HIGH);  // Done
  Serial.println();
}
