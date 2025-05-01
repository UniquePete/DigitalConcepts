/* 
 *  Arduino Pro Mini/RFM95W with BME280E sensor
 *  Site configuration details in <LangloLoRa.h> (arduino/libraries)
 *  Individual node configurations maaintained in <nodeXconfig.h> (arduino/libraries)
 *  Packet structures managed through <PacketHandler.h> (arduino/libraries)
 *  
 */

#include "ProMiniPins.h"	    // Pro Mini Pin Definitions
#include "LowPower.h"         // ? Low Popwer library
#include "SPI.h"              // SPI Interface for RFM95
#include "LoRa.h"             // LoRa class & methods
#include "Wire.h"             // Need this to define second I2C bus (Only one bus on a Pro Mini?)
#include "Adafruit_BME280.h"  // BME280 class & methods
#include "Adafruit_Sensor.h"  // BME280 sensor support methods
#include "LangloLoRa.h"       // LoRa configuration parameters
#include "PacketHandler.h"    // Packet structures
#include "nodeE1config.h"     // Node-specific configuration parameters 

// EEPROM Addressing
const bool eepromSmartAddressing = false;   // EEPROM 2K/4K/8K/16K
//const bool smartAddressing = true;          // EEPROM 32K/64K

// Packet structures
PacketHandler packet;

// Timer control

#define DONE A4a_A3

// BME280 Settings
#define SEALEVELPRESSURE_HPA (1013.25)
const int I2C_BME_Address = 0x76;
const int I2C_EEPROM_Address = 0x50;

union EEPROMPayload {
  uint8_t payloadByte[2];
  uint16_t sequenceNumber;
};

EEPROMPayload EEPROMWriteData, EEPROMReadData;

// Define BME280 sensor
// TwoWire bmeI2C = TwoWire(1);        // Define a second I2C bus to avoid conflict with the OLED display
Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries

const int reportInterval = 60000;             // Report values every 60000 milliseconds (60 seconds)
const int sleepCycles = 8;   // Can only sleep for 8 seconds at a time

int messageCounter = 0;

void setup() {
  pinMode(Builtin_LED,OUTPUT);     // Will use LED to signal packet transmission
  digitalWrite(Builtin_LED, HIGH);  // Turn it off to begin with (although, I don't know why HIGH is off...

  pinMode(DONE, OUTPUT);          // Output signal to control TPL5111 timer
  digitalWrite(DONE, LOW);        // Set the initial state to 'Off'
  
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.print(F("[setup] "));
  Serial.print(myName);
  Serial.println(F(" I2C BME280/LoRa Sensor"));
  Serial.println();

//  Initialise the RFM95 radio
//  SPI.begin(SCK,MISO,MOSI,NSS);
  LoRa.setPins(NSS, RST, DIO0);
  Serial.println(F("[setup] Activate LoRa Sender"));
//  if (!LoRa.begin(BAND, PABOOST)) {
  if (!LoRa.begin(BAND)) {
    Serial.println(F("[setup] Starting LoRa failed!"));
    while (true);
  }

  Serial.print(F("[setup] LoRa Frequency: "));
  Serial.println(BAND);
  
  LoRa.setSpreadingFactor(SpreadingFactor);
  LoRa.setSignalBandwidth(SignalBandwidth);
  LoRa.setPreambleLength(PreambleLength);
  LoRa.setCodingRate4(CodingRateDenominator);
  LoRa.setSyncWord(SyncWord);
  LoRa.disableCrc();
  
  Serial.println(F("[setup] LoRa initialisation complete"));
  Serial.println();

// Initialise the BME280 sensor
  Serial.println(F("[setup] Initialise BME280 sensor"));
//  bmeI2C.begin(BME_I2C_SDA, BME_I2C_SCL, 400000);
//  if (bme280.begin(I2C_BME_Address, &bmeI2C)) {
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println(F("[setup] BME280 sensor initialisation complete"));
  } else {
    Serial.println(F("[setup] Could not find a valid BME280 sensor, check wiring!"));
//    while (true);
  }
  Serial.println();

// Initialise EEPROM access

  Wire.begin();
  Wire.setClock(400000);

  Serial.println(F("[setup] Retrieve Sequence # from EEPROM..."));
  EEPROMReadData.payloadByte[0] = readEEPROMData(0);
  EEPROMReadData.payloadByte[1] = readEEPROMData(1);
  messageCounter = EEPROMReadData.sequenceNumber;

  packet.begin(gatewayMAC,myMAC);
  
  Serial.println(F("[setup] Set-up Complete"));
  Serial.println(""); 

  //  Let everything settle down before continuing
  delay(500);
}

void loop() {
  Serial.println(F("[loop] Send Messages"));
  Serial.println("");
  packet.setSequenceNumber(messageCounter);
  packet.setPacketType(VOLTAGE);
  int vcc = (int) (1000*readVcc());
  packet.setVoltage(vcc);
//  packet.hexDump();
//  packet.serialOut();
  sendMessage();
  // Have a snooze while the gateway porocesses that one
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
  packet.setPacketType(ATMOSPHERE);
  readSensor();
//  packet.hexDump();
//  packet.serialOut();
  sendMessage();
  Serial.println(F("[loop] Increment Message Counter"));
  messageCounter++;                 // Increment message count

//  Store Sequence # in EEPROM
  EEPROMWriteData.sequenceNumber = messageCounter;
  writeEEPROMData(0);

//  Serial.println("[loop] Delay...");
//  Serial.println("");
//  delay(60000);

//  Trigger the TPL5111 timer

  digitalWrite(DONE, HIGH);

//  If the timer is triggered, we should never get here...
//  It seems that 8s is the longest sleep, so need to loop to get to 5 min

  for ( int i = 0; i < sleepCycles; ++i ) {
//    Serial.print("[loop] Sleep cycle ");
//    Serial.println(i);
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

void writeEEPROMData(long eeAddress) {

  Wire.beginTransmission(I2C_EEPROM_Address);

  if ( eepromSmartAddressing ) {
    Wire.write((int)(eeAddress >> 8)); // MSB
    Wire.write((int)(eeAddress & 0xFF)); // LSB
  } else {
    Wire.write( 0 );
  }

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

byte readEEPROMData(long eeaddress) {
  
  Wire.beginTransmission(I2C_EEPROM_Address);

  if ( eepromSmartAddressing ) {
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
  } else {
    Wire.write( eeaddress );
  }

  Wire.endTransmission();

  Wire.requestFrom(I2C_EEPROM_Address, 1);
  delay(1);
  
  byte rdata = 0xFF;
  if (Wire.available()){
    rdata = Wire.read();
//    Serial.print( "[readEEPROMData] Byte: " );
//    Serial.println( rdata );
  } else {
    Serial.println(F("[readEEPROMData] Nothing to read..."));
  }
  return rdata;
}

float readVcc(void) {
  float result = 0;

  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);                         // Wait for Vref to settle

  ADCSRA |= _BV(ADSC);              // Start conversion
  while (bit_is_set(ADCSRA,ADSC));  // Wait conversion is ready
 
  uint8_t low  = ADCL; // Read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
  result = (float)((high<<8) | low);
 
  result = 1125300L/result;  // Calculate battery voltage in mV
  Serial.print(F("[readVcc] Voltage: "));
  Serial.print(result/1000);
  Serial.println(F(" V"));
  return (result/1000);      // Return battery voltage in V
};

void readSensor() {
  
  int temperature = (int) (10*bme280.readTemperature());
  uint16_t pressure = (int) (bme280.readPressure() / 91.79F);
  uint16_t humidity = (int) bme280.readHumidity();

  packet.setTemperature(temperature);
  packet.setPressure(pressure);
  packet.setHumidity(humidity);

  Serial.println(F("[updateData] Sensor read..."));
  Serial.print(F("[updateData] Temperature: "));
  Serial.print((float) temperature/10, 1);
  Serial.println(F(" °C"));
  Serial.print(F("[updateData] Pressure: "));
  Serial.print(pressure);
  Serial.println(F(" hPa"));
  Serial.print(F("[updateData] Humidity: "));
  Serial.print(humidity);
  Serial.println(F(" %"));
  Serial.println("");
}

void sendMessage() {

// If the host is little endian, we need to reverse the byte order before sending

//  Serial.println(F("[sendMessage] Sending packet: "));
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

//  Serial.println("[sendMessage] Finished Packet");

  LoRa.endPacket();             // Finish packet and send it
//  Serial.println(F("[sendMessage] Packet Sent"));
  digitalWrite(Builtin_LED, HIGH);  // Done
//  Serial.println();
}
