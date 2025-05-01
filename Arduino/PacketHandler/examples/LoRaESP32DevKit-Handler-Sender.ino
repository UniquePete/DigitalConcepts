/*
 * Note that this sketch supports the original breadboard wiring, not the ultimate single
 * I2C bus set-up with wiring that is generally compatible with other ESP32 configurations
 */

#include "ESP32DevKitPins.h"  // ESP32 Dev Kit Pin Definitions
#include "SPI.h"              // SPI Interface for RFM95
#include "LoRa.h"             // LoRa class & methods
#include "CRC32.h"            // Checksum methods
#include "Adafruit_BME280.h"  // BME280 class & methods
#include "Adafruit_Sensor.h"  // BME280 sensor support methods
#include "Wire.h"             // Need this to define second I2C bus
#include "Ticker.h"           // Ticker class & methods
#include "LangloLoRa.h"       // LoRa configuration parameters
#include "PacketHandler.h"    // Packet structures
#include "nodeA1Dconfig.h"    // Node-specific configuration parameters

// Packet structure
PacketHandler packet;

// Define OLED Display
const int I2C_Display_Address = 0x3c;

// BME280 Settings
const int I2C_BME_Address = 0x77;   // Not actually necessary if using the SparcFun sensor @ 0x77
Adafruit_BME280 bme280;             // Defaults (I2C_BME_Address = 0x77) set within Adafruit libraries

uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)
uint16_t inputRainfall = 0;
int adc0 = 0;

uint16_t windBearing = 0;
uint8_t windSpeed = 0;

const int reportInterval = 60000;     // Report values every 60000 milliseconds (60 seconds)

// Define the update timer
Ticker dataTicker;

uint16_t messageCounter = 0;

void setup() {
  pinMode(Builtin_LED,OUTPUT);     // Will use LED to signal packet transmission
  digitalWrite(Builtin_LED, HIGH);  // Turn it off to begin with (although, I don't know why HIGH is off...
  
  Serial.begin(115200);
  while (!Serial);        // If just the the basic function, must connect to a computer
    delay(500);
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.print(" (0x");
  Serial.print(myMAC,HEX);
  Serial.print(")");
  Serial.println(" I2C BME280/LoRa Sensor");
  Serial.println();
  
//  Initialise the RFM95 radio
  SPI.begin(SCK,MISO,MOSI,NSS);
  LoRa.setPins(NSS,RST,DIO0);
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

  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
/*  
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
*/
  packet.begin(gatewayMAC,myMAC);

  // Let the gateway/broker know there's been a reset
  
  packet.setPacketType(RESET);
  packet.setResetCode(0);
  packet.serialOut();
  sendMessage();
 
  Serial.println("[setup] Set-up Complete");
  Serial.println(""); 

  //  Let everything settle down before continuing
  delay(500);
}

void loop() {
  Serial.print("[loop] Sequence # ");
  Serial.println(messageCounter);
  packet.setSequenceNumber(messageCounter);
  packet.setPacketType(WEATHER);
  readSensor();
  Serial.print("[loop] ");
  Serial.print(myName);
  Serial.print(" (0x");
  Serial.print(myMAC,HEX);
  Serial.println(") sending Weather messages...");
  sendMessage();
  packet.hexDump();
  Serial.println("[loop] Read sensor a second time...");
  readSensor();
  Serial.println("[loop] Increment Message Counter");
  messageCounter++;                 // Increment message count
  Serial.println("[loop] Delay...");
  Serial.println(""); 
  delay(reportInterval);
}

void readSensor() {
  Wire.begin(SDA,SCL);
  if (bme280.begin(I2C_BME_Address)) {
    Serial.println("[setup] BME280 sensor initialisation complete");
  } else {
    Serial.println("[setup] Could not find a valid BME280 sensor, check wiring!");
//    while (true);
  }
  
  temperature = (int) (10*bme280.readTemperature());
  pressure = (int) (bme280.readPressure() / 91.79F);
  humidity = (int) (bme280.readHumidity());

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
  inputRainfall = 155; // Dummy value (in 1/10 mm) just so taht there's somethign other than 0 to report

  packet.setRainfall(inputRainfall);

  Serial.print("[readSensor] Rainfall: ");
  Serial.println((float) inputRainfall/10, 1);

/*
 * When we've actually got a sensor to read, these variables should be set to the values
 * read from the sensor(s)
 */
  windBearing = 270;
  windSpeed = 10;

  packet.setWindBearing(windBearing);
  packet.setWindSpeed(windSpeed);

  Serial.print("[readSensor] Wind Bearing: ");
  Serial.println(windBearing);
  Serial.print("[readSensor] Wind Speed: ");
  Serial.println(windSpeed);
}

void sendMessage() {

//  Serial.println("[sendMessage] Outgoing packet: ");
  digitalWrite(Builtin_LED, LOW);  // Turn on the LED to signal sending

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
*/
//  Serial.println("[sendMessage] Finished Packet");
  LoRa.endPacket();             // Finish packet and send it
  Serial.println("[sendMessage] Packet Sent");
  digitalWrite(Builtin_LED, HIGH);  // Done
}
