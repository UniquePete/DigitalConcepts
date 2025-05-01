/* Tank Node
 
   Function:
   Read water level and temperature sensors and forward data to LoRa Gateway
  
   Description:
   1. Fundamentally based on Heltec CubeCell pingpong code
   2. Uses packet structures defined in the PacketHandler library
   3. Communicates data to [Heltec WiFi LoRa 32 / Raspberry Pi] LoRa Gateway
   4. Senses and reports distance measurement recorded with a JA-SR04M based ultrasonic distance sensor
 
   24 Oct 2023 1.0.0  Initial implementation based on AWTS Node 2.0
                      Migrate EEPROM management to EepromHandler
   22 Nov 2023 1.1.0  Improve measurement methodology (tankStatus()) to dampen fluctuations
   25 Nov 2023 1.1.1  Further dampening refinements
                      Change distance measurements from cm to mm
                      Change to standardised software revision format
   03 Apr 2024 1.2.0  Send distance measurement rather than percentage full
                      Read configuration data from EEPROM
                      Add TPL5010 watchdog timer support
                      Use CubeCell_NeoPixel.h library and move NeoPixel 'flashing' to sendMessage()
                      Rationalise Vext control settings - Vext need only be turned OFF when transitioning to LOWPOWER mode
                      Rationalise use of Wire.begin()/Wire.end()
   02 May 2024 1.2.1  Use speed of sound to 3 decimal places (rather than just 2)
  
   Digital Concepts
   2 May 2024
   digitalconcepts.net.au
 */

#include <Arduino.h>
#include <CubeCell_NeoPixel.h>  // CubeCell NeoPixel control library
#include <LoRa_APP.h>           // CubeCell LoRa
#include <Wire.h>               // I2C Bus
#include <OneWire.h>            // Required for Dallas Temperature (but make sure to use the Heltec CubeCell version!)
#include <DallasTemperature.h>  // DS18B20 Library
#include <CRC32.h>              // Checksum methods
#include <EepromHandler.h>      // [Langlo] EEPROM management
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
softwareRevision sketchRevision = {1,2,1};
String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

// NeoPixel Parameters: # pixels, RGB or RGBW device, device colour order + frequency

CubeCell_NeoPixel neo(1, RGB, NEO_GRB + NEO_KHZ800);

// Instantiate the objects we're going to need and declare their associated variables

EepromHandler eeprom;
PacketHandler packet;

uint8_t* descriptor;
uint16_t messageCounter = 0;

// Battery
uint16_t batteryVoltage = 0;

// DS18B20
#define oneWireBus              A4a_A2
#define temperaturePrecision    9

int16_t nodeTemperature = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensors(&oneWire);

// JA-SR04M pins
#define echoPin ultraEcho
#define trigPin ultraTrig

uint8_t tankIndex;            // Set during setup
int tankHeight = 1500;        // Tank height [mm]
int waterLevelOffset = 170;   // Zero point (minimum measurable distance [mm])
uint16_t waterLevel = 0;      // Distance to water level (mm)
uint16_t tankLevel = 0;       // % Full

#define sampleInterval 100
#define numberOfSamples 21    // 11 samples was not quite enough, there were still periodic glitches
#define medianIndex 10        // the 'middle' of numberOfSamples
unsigned long sample[numberOfSamples];

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
 *  The CubeCell LoRa Radio library sets its SyncWord in the file LoRaMac.h, through the definition of
 *  LORA_MAC_PRIVATE_SYNCWORD, which is set to 0x12, but this does not appear to be accessible elsewhere.
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
 
  // Get the Tank ID
  tankIndex = eeprom.readUint8(EH_TANKID);
 
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
      Serial.print("[loop-TX] Read and increment Sequence Number  ");
      digitalWrite(Vext, LOW);    // Turn on the external power supply
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();
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

      tankStatus();
      packet.setPacketType(TANK);
      packet.setTankId(tankIndex);
      packet.setTankLevel(waterLevel);
//      packet.hexDump();
//      packet.serialOut();   
            
      sendMessage();

      eeprom.writeUint16(EH_SEQUENCE, messageCounter); // Save the message counter

      Serial.println(""); 
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

void sortSampleArray() {
  int i, j, x;
  i = 1;
  while (i < numberOfSamples) {
    x = sample[i];
    j = i - 1;
    while ((j >= 0) && (sample[j] > x)) {
      sample[j+1] = sample[j];
      j--;
    }
    sample[j+1] = x;
    i++;
  }
}

void tankStatus () {
  /*
    At the moment, we do all the work here: measure the time delay for the ultrasonic pulse,
    calculate the distance it travelled, then work out how full the tank is based on settings
    for the tank height and calibration factor. Ultimately, all we want to do here is measure
    the pulse delay and send that to the monitor. Thereafter, the monitor can decide how it
    wants to deal with that information, based on its knowledge of the physical environment — 
    the tank height and any calibration factor - that needs to be taken into consideration in
    calculating the result that is ultimately displayed.
  */
  bool keepGoing = true;
  int loopCycles = 0;
  unsigned long t = 0;

  unsigned long duration = 0;       // Sound wave travel time
  
//  Serial.println("[tankStatus] Startup is complete");

  // Clear the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Get smoothed value from the dataset
  // We currently take 11 measurements, sort them in order of magnitude, then
  // use the median to calculate the distance travelled. In practice, this gives
  // a much more consistent result than using the average, allowing us to make more
  // reliable decisions that are based on the level of water in the tank.

  while (keepGoing) {
    if (millis() > t + sampleInterval) {
      t = millis();        
      // Take a depth measurement
      // Set the trigPin HIGH (ACTIVE) for 10 microseconds
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      
      // Read the echoPin, returns the sound wave travel time in microseconds
      sample[loopCycles] = pulseIn(echoPin, HIGH);
      Serial.print("[tankStatus] Duration [");
      Serial.print(loopCycles);
      Serial.print("]: ");
      Serial.println(sample[loopCycles]);

      if (++loopCycles >= numberOfSamples) {  // Take several samples before returning a reading
        keepGoing = false;
        Serial.print("[tankStatus] Total readings: ");
        Serial.println(loopCycles);
        sortSampleArray();
        duration = sample[medianIndex];
        unsigned long sampleMax = duration;
        unsigned long sampleMin = duration;
        for (int i = 0; i < numberOfSamples; i++) {
          if (sample[i] > sampleMax) sampleMax = sample[i];
          if (sample[i] < sampleMin) sampleMin = sample[i];
        }
        Serial.print("[tankStatus] Sample Interval: ");
        Serial.println(sampleInterval);
        Serial.print("[tankStatus]    Max Duration: ");
        Serial.println(sampleMax);
        Serial.print("[tankStatus]    Min Duration: ");
        Serial.println(sampleMin);
        Serial.print("[tankStatus]    Sample Range: ");
        Serial.println(sampleMax - sampleMin);
        Serial.print("[tankStatus] Median Duration: ");
        Serial.println(duration);
      }
    }
  }
 
  // Calculate the water level
  waterLevel = (duration * 0.343 / 2) + 0.5;  // Speed of sound wave divided by 2 (out and back)
  Serial.print("[tankStatus] Distance to water level: ");
  Serial.print(waterLevel);
  Serial.println(" mm");

  // All we send out now is [the distance from the sensor to] the water level - we leave the calculations up to the receiver
  // The following just remain to show what should be done to determine the tank % full

  if (waterLevel > (tankHeight + waterLevelOffset)) waterLevel = tankHeight + waterLevelOffset;  // This shouldn't happen, but...
  tankLevel = (100 * (tankHeight - (waterLevel - waterLevelOffset)) / tankHeight) + 0.5;
  Serial.print("[tankStatus]             Tank Height: ");
  Serial.print(tankHeight);
  Serial.println(" mm");
  Serial.print("[tankStatus]              Tank Level: ");
  Serial.print(tankLevel);
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