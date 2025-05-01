/* Langlo AWTS Node

   Function:
   Read pressure and water level sensors and forward data to LoRa Gateway

   Description:
   1. Fundamentally based on Heltec CubeCell pingpong code
   2. Uses packet structures defined in the PacketHandler library
   3. Communicates data to [Heltec WiFi LoRa 32 / Raspberry Pi] LoRa Gateway;

   16 May 2022 1.0.0  Initial implementation
   27 Aug 2022 2.0.0  Refactored to use PacketHandler library
   03 Apr 2024 2.1.0  Migrate EEPROM management to EepromHandler
                      Add TPL5010 watchdog timer support
                      Use CubeCell_NeoPixel.h library and move NeoPixel 'flashing' to sendMessage()
                      Rationalise Vext control settings - Vext need only be turned OFF when transitioning to LOWPOWER mode
                      Rationalise use of Wire.begin()/Wire.end()

   Digital Concepts
   3 Apr 2024
   digitalconcepts.net.au
 
 */

#include <Arduino.h>
#include <CubeCell_NeoPixel.h>  // CubeCell NeoPixel control library
#include <LoRa_APP.h>           // CubeCell LoRa
#include <Wire.h>               // I2C Bus
#include <HX711_ADC.h>          // Pressure Sensor
#include <OneWire.h>            // Required for Dallas Temperature (but make sure to use the Heltec CubeCell version!)
#include <DallasTemperature.h>  // DS18B20 Library
#include <CRC32.h>              // Checksum methods
#include <EepromHandler.h>		  // EEPROM management
#include <PacketHandler.h>      // [Langlo] LoRa Packet management

#include "LangloLoRa.h"         // LoRa configuration parameters
#include "CubeCellPlusPins.h"   // CubeCell Plus pin definitions

// Watch dog timer reset pin
#define wdtDone GPIO12

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {2,1,0};
String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

// NeoPixel Parameters: # pixels, RGB or RGBW device, device colour order + frequency

CubeCell_NeoPixel neo(1, RGB, NEO_GRB + NEO_KHZ800);

// Instantiate the objects we're going to need and declare their associated variables

EepromHandler eeprom;
PacketHandler packet;

uint8_t* descriptor;
uint16_t messageCounter = 0;

// Battery
const float voltageFactor = 4.026/1024;  // Measuring voltage across 22k/100k voltage divider (actually up to ~4.2V)
int inputValue = 0;
uint16_t batteryVoltage = 0;

// DS18B20
#define oneWireBus              A4a_A2
#define temperaturePrecision    9

int16_t nodeTemperature = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensors(&oneWire);

// HX710B/HX711 pins
const int HX711_dout = sphygOUT; // mcu > HX711 OUT pin
const int HX711_sck = sphygSCK;  // mcu > HX711 SCK pin

// HX710B/HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

static boolean newDataReady = false;
int16_t blowerPressure = 0;
uint16_t itLevel = 0;
boolean _tare = false;                 // Set this to false if you don't want tare to be performed in the next step
unsigned long stabilisingTime = 2000;  // Tare preciscion can be improved by adding a few seconds of stabilizing time
// float calibrationValue = 696.0;     // Comment this out if you want to set this value in the sketch
float calibrationValue = 1000000.0;    // Calibration value

// JA-SR04M pins
#define echoPin ultraEcho
#define trigPin ultraTrig

int itHeight = 150;                    // Influent Tank height [cm]
int itLevelOffset = 17;                // Zero point (minimum measurable disatance [cm])

/* 
 *  The following parameters need to be synchronised with those specified in LangloLoRa.h, the latter being used
 *  for all processors other than the CubeCell
 *  
 *  #define BAND 917E6
 *  #define SignalBandwidth 125E3
 *  #define SpreadingFactor 7
 *  #define CodingRateDenominator 5
 *  #define PreambleLength 8
 *  
 *  At this stage, I dont know where the CubeCell software sets its SyncWord, or whether it's just set at 0x12
 */

static RadioEvents_t RadioEvents;

// LowPower stuff

#define sleepTime 60000
static TimerEvent_t wakeUp;

#define ADC_CTL 7

typedef enum {
    LOWPOWER,
    TX
} States_t;

States_t state;

int16_t rssi,rxSize;

void onWake() {
  Serial.println(); 
  Serial.println("[onWake] Wake and process sensor data");
  state = TX;
}

void setup() {
  boardInitMcu( );
  Serial.begin(115200);
  delay(200);

  // Initialise the EEPROM Handler
  
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW); // Activate Vext (we're really writing to Vext_Ctrl here)
  Wire.begin();
  eeprom.begin(&Wire);
  
  Serial.print("[setup] ");
  descriptor = eeprom.readBytes(EH_DESCRIPTOR);
  int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
  for (int i = 0; i < byteCount; i++) {
    Serial.print((char)descriptor[i]);
  }
  Serial.println();
  Serial.println("[setup]        Sketch " + sketchRev);
  Serial.println("[setup] PacketHandler " + packet.softwareRevision(PACKET_HANDLER));
  Serial.println("[setup]   NodeHandler " + packet.softwareRevision(NODE_HANDLER));
  Serial.println("[setup] EepromHandler " + eeprom.softwareRevision());
  Serial.println();

  batteryVoltage=0;
  rssi=0;

  RadioEvents.TxDone = onTxDone;
  RadioEvents.TxTimeout = onTxTimeout;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( Frequency );
  Radio.SetTxConfig( MODEM_LORA, OutputPower, 0, SignalBandwidthIndex,
                                 SpreadingFactor, CodingRate,
                                 PreambleLength, FixedLengthPayload,
                                 true, 0, 0, IQInversion, 3000 );

  Radio.Sleep( );
    
  Serial.println("[setup] LoRa initialisation complete");
  
  TimerInit( &wakeUp, onWake );
  onWake();

  // JA-SR04M setup
  pinMode(trigPin, OUTPUT);   // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT);    // Sets the echoPin as an INPUT
  
  // initialise the Packet Handler
  
  packet.begin(eeprom.readUint32(EH_GATEWAY_MAC),eeprom.readUint32(EH_NODE_MAC));

  // Let the gateway/broker know there's been a reset
  
  packet.setPacketType(RESET);
  packet.setResetCode(0);
  packet.serialOut();
  sendMessage();

  // Have a snooze while the gateway hadndles that one
  delay(100);
  
  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop() {
//  Serial.println("[loop] Enter Loop");
	switch ( state ) {
		case TX: {
      Serial.print("[loop-TX] Read and increment Sequence Number       ");
      digitalWrite(Vext, LOW);    // Turn on the external power supply
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();               // On with the show...
      messageCounter = eeprom.readUint16(EH_SEQUENCE) + 1;      
      Serial.println(messageCounter);

      batteryVoltage=getBatteryVoltage();
      packet.setSequenceNumber(messageCounter);
      packet.setPacketType(VOLTAGE);
      packet.setVoltage(batteryVoltage);
//      packet.hexDump();
//      packet.serialOut();

      // Send the message    
      sendMessage();
     
      // Have a snooze while the gateway hadndles that one
      delay(100);

      readDS18B20Sensors();
      packet.setPacketType(TEMPERATURE);
      packet.setTemperature(nodeTemperature);
//      packet.hexDump();
//      packet.serialOut();   
            
      sendMessage();

      // No need to snooze here because it takes a while to collect the following data

      awtsStatus();
      packet.setPacketType(AWTS);
      packet.setBlowerPressure(blowerPressure);
      packet.setItLevel(itLevel);
//      packet.hexDump();
//      packet.serialOut();   
            
      sendMessage();
 
      eeprom.writeUint16(EH_SEQUENCE, messageCounter); // Save the message counter

      Serial.println(); 
      state=LOWPOWER;
	    break;
    }
		case LOWPOWER: {
      digitalWrite(Vext, HIGH);               // Turn off the external power supply before sleeping
      TimerSetValue( &wakeUp, sleepTime );
      TimerStart( &wakeUp );
      lowPowerHandler();
      break;
    }
    default:
        break;
	}
//    Radio.IrqProcess( );
}

void onTxDone(void)
{
  Serial.println("[OnTxDone] TX done!");
  Radio.Sleep( );
}

void onTxTimeout(void)
{
  Serial.println("[OnTxTimeout] TX Timeout...");
  Radio.Sleep( );
}

void readDS18B20Sensors() {
  Serial.println("[readDS18B20Sensors] Read sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/
  ds18b20Sensors.begin();
  delay(100);                 // Give everything a moment to settle down
  
  ds18b20Sensors.requestTemperatures(); // Send the command to get temperatures
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.println("[readDS18B20Sensors] Device 1 (index 0)");

  float tempTemperature = ds18b20Sensors.getTempCByIndex(0);
  Serial.print("[readDS18B20Sensors] Returned value: ");
  Serial.println(tempTemperature);

  nodeTemperature = (int) (10*(tempTemperature + 0.05));
 
  Serial.print("[readDS18B20Sensors] Temperature: ");
  Serial.println((float) nodeTemperature/10, 1);
}

void awtsStatus () {
  const int sampleInterval = 500;  // Increase value to slow down activity
  const int loopCycleLimit = 4;    // # samples before recording value
  bool keepGoing = true;
  int loopCycles = 0;
  unsigned long t = 0;
  float sensorValue = 0.0;

  long duration = 0;                   // Sound wave travel time
  int distance = 0;                    // Distance measurement

  digitalWrite(Vext, LOW);         // Make sure the external power supply is on
  delay(100);                      // Give everything a moment to settle down
  
  LoadCell.begin();
  LoadCell.start(stabilisingTime, _tare);
  LoadCell.setCalFactor(calibrationValue);
//  Serial.println("[awtsStatus] Startup is complete");

  // Clear the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  while (keepGoing) {
    // Check for new data/start next conversion:
    if (LoadCell.update()) {
      newDataReady = true;
    }
    // Get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + sampleInterval) {
        sensorValue = LoadCell.getData();
        Serial.print("[awtsStatus] Pressure reading: ");
        Serial.println(sensorValue);
        newDataReady = false;
        t = millis();
          
        // Take an IT depth measurement while we're waiting
        // Set the trigPin HIGH (ACTIVE) for 10 microseconds
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        // Read the echoPin, returns the sound wave travel time in microseconds
        duration += pulseIn(echoPin, HIGH);
        Serial.print("[awtsStatus] Distance: ");
        Serial.println((duration/(loopCycles+1)) * 0.034 / 2);

        if (++loopCycles > loopCycleLimit) {  // Take several samples before returning a reading
          keepGoing = false;
          blowerPressure = (int16)(100 * sensorValue);
          Serial.print("[awtsStatus] Total readings: ");
          Serial.println(loopCycles);
          duration = (long)(duration / loopCycles);
        }
      }
    }
  }
  newDataReady = false;
 
  // Calculate the IT level
  distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (out and back)
  Serial.print("[awtsStatus] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  if (distance > (itHeight + itLevelOffset)) distance = itHeight + itLevelOffset;  // This shouldn't happen, but...
  itLevel = (int) (100 * (itHeight - (distance-itLevelOffset)) / itHeight) + 0.5;
  Serial.print("[awtsStatus] IT Height: ");
  Serial.print(itHeight);
  Serial.println(" cm");
  Serial.print("[awtsStatus] IT Level: ");
  Serial.print(itLevel);
  Serial.println(" %");
}

void blowerStatus () {
  const int sampleInterval = 500;  // Increase value to slow down activity
  const int loopCycleLimit = 4;    // # samples before recording value
  bool keepGoing = true;
  int loopCycles = 0;
  unsigned long t = 0;
  float sensorValue = 0.0;
  
  digitalWrite(Vext, LOW);        // Make sure the external power supply is on
  delay(100);                     // Give everything a moment to settle down
  
  LoadCell.begin();
  LoadCell.start(stabilisingTime, _tare);
  LoadCell.setCalFactor(calibrationValue);
//  Serial.println("[awtsStatus] Startup is complete");

  while (keepGoing) {
    // Check for new data/start next conversion:
    if (LoadCell.update()) {
      newDataReady = true;
    }
    // Get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + sampleInterval) {
        sensorValue = LoadCell.getData();
        Serial.print("[blowerStatus] Load_cell output val: ");
        Serial.println(sensorValue);
        newDataReady = false;
        t = millis();        
        if (++loopCycles > loopCycleLimit) {  // Take several samples before returning a reading
          keepGoing = false;
          blowerPressure = (int16)(100 * sensorValue);
        }
      }
    }
  }
  newDataReady = false;
 }

void itStatus () {
  long duration = 0;                   // Sound wave travel time
  int distance = 0;                    // Distance measurement

  // Clear the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (out and back)
  Serial.print("[itStatus] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  itLevel = (int) 100 * (itHeight - (distance-itLevelOffset)) / itHeight;
  Serial.print("[itStatus] IT Height: ");
  Serial.print(itHeight);
  Serial.println(" cm");
  Serial.print("[itStatus] IT Level: ");
  Serial.print(itLevel);
  Serial.println(" %");
}

void sendMessage() {
/*
 * The necessary payload content must be set by the relevant sensor method(s).
 * At this point, we assume that everything is ready to go, the Packet Handler having
 * assembled what it has in the nominated [packetType] format.
 */
 
// Only do this if you want to see what's being sent
// packet.hexDump();    // Hex dump
// packet.serialOut();  // Plain English

  int totalByteCount = packet.packetByteCount();

  Serial.println("[sendMessage] Sending packet...");
  neoPixel(0,32,0);   // NeoPixel [low intensity] GREEN
  Radio.Send((uint8_t *)packet.byteStream(), packet.packetByteCount());
  neoPixel(0,0,0);    // NeoPixel OFF
  Serial.println("[sendMessage] Packet sent");  
  Serial.println();
  resetWatchdogTimer();
}

void neoPixel(uint8_t red, uint8_t green, uint8_t blue) {
	neo.begin();                                        // Initialise RGB strip object
	neo.clear();                                        // Set all pixel 'off'
	neo.setPixelColor(0, neo.Color(red, green, blue));  // The first parameter is the pixel index, and we only have one
	neo.show();                                         // Send the updated pixel colors to the hardware.
}

void resetWatchdogTimer() {
  Serial.println("[resetWatchdogTimer] Pat the dog...");
  digitalWrite(wdtDone,HIGH);
  digitalWrite(wdtDone,LOW);
}