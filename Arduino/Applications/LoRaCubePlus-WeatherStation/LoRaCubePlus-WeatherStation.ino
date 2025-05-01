/* Langlo Weather Station Node
  
   Function:
   Read sensors and forward data to LoRa Gateway
   
   Description:
   1. Fundamentally based on Heltec CubeCell pingpong code
   2. Uses packet structures defined in the PacketHandler library
   3. Communicates data to [Heltec WiFi LoRa 32 / Raspberry Pi] LoRa Gateway
  
   21 Feb 2022 1.0.0  Initial implementation
   27 Aug 2022 2.0.0  Refactored to use PacketHandler library
   29 Nov 2023 2.1.0  Migrate EEPROM management to EepromHandler
                      Add code to manage rainfall counter debouncing
   01 Apr 2024 2.1.1  Correct I2C bus activation sequence (BME280 seems to require a reset - end/begin, not just begin)
   02 Apr 2024 2.2.0  Add TPL5010 watchdog timer support
               2.2.1  Use CubeCell_NeoPixel.h library and move NeoPixel 'flashing' to sendMessage()
                      Rationalise Vext control settings - Vext need only be turned OFF when transitioning to LOWPOWER mode
                      Rationalise use of Wire.begin()/Wire.end()
   05 Apr 2024 2.2.2  Minor changes to Serial Monitor messages
   18 Apr 2024 2.2.3  Rework I2C bus management
   18 Apr 2024 2.3.0  Use interrupt to signal display software revision levels
   06 May 2024 2.3.1  Increase debounce interval from 50ms to 200ms
                      Move rain counter initialisation to setup()
                      Add some more Serial Monitor output to help with the debugging process
   31 Aug 2024 2.3.2  Rearrange measureWind function to attempt to address unusual (zero kph) wind speed measurements
                      Reduce debounce interval to 50ms (more likely the problem!)

   Digital Concepts
   31 August 2024
   digitalconcepts.net.au
 */

#include <Arduino.h>
#include <CubeCell_NeoPixel.h>  // CubeCell NeoPixel control library
#include <LoRa_APP.h>           // CubeCell LoRa
#include <Wire.h>               // I2C Bus
#include <HT_SH1107Wire.h>      // CubeCell Plus display library
#include <Seeed_BME280.h>       // BME280 Sensor
#include <OneWire.h>            // Required for Dallas Temperature
#include <DallasTemperature.h>  // DS18B20 Library
#include <CRC32.h>              // Checksum generator
#include <EepromHandler.h>		  // EEPROM management
#include <PacketHandler.h>		  // [Langlo] LoRa Packet management

#include "LangloLoRa.h"         // LoRa parameters
#include "CubeCellPlusPins.h"   // CubeCell Plus pin definitions

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {2,3,2};
String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

// NeoPixel Parameters: # pixels, RGB or RGBW device, device colour order + frequency

CubeCell_NeoPixel neo(1, RGB, NEO_GRB + NEO_KHZ800);

// Instantiate the objects we're going to need and declare their associated variables

EepromHandler eeprom;
PacketHandler packet;

uint8_t* descriptor;
uint16_t messageCounter = 0;

SH1107Wire  boardDisplay(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10), *displayPtr; // addr, freq, sda, scl, resolution, rst

// Now set up the variables for the various sensors
// Battery
const float voltageFactor = 4.026/1024;  // Measuring voltage across 22k/100k voltage divider (actually up to ~4.2V)
int inputValue = 0;
uint16_t batteryVoltage = 0;

// DS18B20
#define oneWireBus              A4a_A2
#define temperaturePrecision    9

int16_t nodeTemperature = 0;

uint16_t rainCounter = 0;
unsigned long previousRainInterrupt = 0;
uint16_t windBearing = 0;
uint16_t windCounter = 0;
uint16_t windSpeed = 0;
unsigned long previousWindInterrupt = 0;
unsigned long debounceInterval = 40;      // 40 mSec (should allow wind speed measurements up to 90 kph)

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensors(&oneWire);

// BME280
BME280 bme280;
uint16_t humidity = 0;
uint16_t pressure = 0;
uint16_t temperature = 0;

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
#define displayTime 30000
static TimerEvent_t displaySleep;

typedef enum {
    LOWPOWER,
    TX
} States_t;

States_t state;

int16_t rssi,rxSize;

void onWake() {
  Serial.println(); 
  Serial.println("[onWake] Wake and set TX state...");

  digitalWrite( Vext, LOW );  // Turn on power to EEPROM & sensors
  delay(100);                 // Give everything a moment to settle down
  Wire.end();                 // The Wire instance seems to need to be reset before use...
  Wire.begin();
  eeprom.begin( &Wire );

  state = TX;
}

void setup() {
  boardInitMcu( );
  Serial.begin(115200);
  delay(200);

  // Initialise the EEPROM Handler
  
  pinMode( Vext, OUTPUT );
  digitalWrite( Vext, LOW ); // Activate Vext (we're really writing to Vext_Ctrl here)
  Wire.begin();
  eeprom.begin( &Wire );
  
  Serial.print("[setup] ");
  descriptor = eeprom.readBytes( EH_DESCRIPTOR );
  int byteCount = eeprom.getParameterByteCount( EH_DESCRIPTOR );
  for (int i = 0; i < byteCount; i++) {
    Serial.print((char)descriptor[i]);
  }
  Serial.println();
  Serial.println("[setup]        Sketch " + sketchRev);
  Serial.println("[setup] PacketHandler " + packet.softwareRevision( PACKET_HANDLER ));
  Serial.println("[setup]   NodeHandler " + packet.softwareRevision( NODE_HANDLER ));
  Serial.println("[setup] EepromHandler " + eeprom.softwareRevision());
  Serial.println();

/*
  displayPtr = &boardDisplay;
  boardDisplay.init();
  boardDisplay.setFont(ArialMT_Plain_10);
  boardDisplay.screenRotate(ANGLE_180_DEGREE);
  boardDisplay.clear();
  boardDisplay.setFont(ArialMT_Plain_16);
  String messageString = "Testing...";
  boardDisplay.drawString(10, 20, messageString);
  boardDisplay.display();
*/
  PINMODE_INPUT_PULLUP( rainCollectorPin );
  attachInterrupt( rainCollectorPin, incrementRainCount, FALLING );
  PINMODE_INPUT_PULLUP( windSpeedPin );
  PINMODE_INPUT_PULLUP( interruptButton );
  attachInterrupt( interruptButton, displayRevisionLevels, FALLING );

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
  TimerSetValue( &wakeUp, sleepTime );
  TimerInit( &displaySleep, displayOff );
  TimerSetValue( &displaySleep, displayTime );
  state = TX;
  
  // Initialise the Packet Handler
  
  packet.begin( eeprom.readUint32( EH_GATEWAY_MAC ), eeprom.readUint32( EH_NODE_MAC ));

  // and the Rain Counter
   
  rainCounter = eeprom.readUint16( EH_RAINFALL );
  
  // Let the gateway/broker know there's been a reset
  
  packet.setPacketType( RESET );
  packet.setResetCode( 0 );
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
      messageCounter = eeprom.readUint16( EH_SEQUENCE ) + 1;      
      Serial.println( messageCounter );

      Serial.println("[loop-TX] Read battery voltage...");
      batteryVoltage = getBatteryVoltage();
      Serial.print("[loop-TX] Battery voltage: ");
      Serial.println((float) batteryVoltage/1000, 2);
      packet.setSequenceNumber( messageCounter );
      packet.setPacketType( VOLTAGE );
      packet.setVoltage( batteryVoltage );
//      packet.hexDump();
//      packet.serialOut();

      // Send the message
      sendMessage();
     
      // Have a snooze while the gateway hadndles that one
      delay(100);
/*      
      readDS18B20Sensors();
      packet.setPacketType(TEMPERATURE);
      packet.setTemperature(nodeTemperature);
//      packet.hexDump();
//      packet.serialOut();   
            
      sendMessage();
      
      // Have a snooze while the gateway hadndles that one
      delay(100);
*/
      packet.setPacketType( WEATHER );      
      packet.setRainfall(eeprom.readUint16( EH_RAINFALL ));   

      Serial.print("[loop-TX] Rain Counter : ");
      Serial.println(rainCounter);
      Serial.print("[loop-TX] EH_RAINFALL : ");
      Serial.println(eeprom.readUint16( EH_RAINFALL ));

      measureWind();
      packet.setWindBearing( windBearing );
      packet.setWindSpeed( windSpeed );
      readBMESensor();
      packet.setTemperature( temperature );
      packet.setPressure( pressure );
      packet.setHumidity( humidity );
      sendMessage();

      eeprom.writeUint16( EH_SEQUENCE, messageCounter ); // Save the message counter

      Serial.println(); 
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
  Serial.println("[readDS18B20Sensors] Read sensors...");
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

  nodeTemperature = (int) (10*tempTemperature);
 
  Serial.print("[readDS18B20Sensors] Temperature: ");
  Serial.println((float) temperature/10, 1);
}

void readBMESensor() {
  Serial.println("[readBMESensor] Read sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/
  Wire.begin();

  if (bme280.init(0x76)) {
    Serial.println( "[readBMESensor] BME sensor [0x76] initialisation complete" );
  } else if (bme280.init(0x77)) {
    Serial.println( "[readBMESensor] BME sensor [0x77] initialisation complete" );
  } else {
    Serial.println( "[readBMESensor] BME sensor initialisation error" );
  }

  delay(100); // The BME280 needs a moment to get itself together... (50ms is too little time)
 
  temperature = (int) (10*bme280.getTemperature());
  pressure = (int) (bme280.getPressure() / 91.79F);
  humidity = (int) (bme280.getHumidity());

  Serial.print("[readBMESensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readBMESensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readBMESensor] Humidity: ");
  Serial.println(humidity);
}

void incrementRainCount() {
  unsigned long time = 0;
  time = millis();
  if (( time - previousRainInterrupt ) > debounceInterval ) {
    Serial.println("[incrementRainCount] Increment rain counter...");
//    Serial.print("         was: ");
    digitalWrite(Vext, LOW);    // Make sure the external power supply is on
    Wire.end();                 // We seem top need to this first...
    Wire.begin();               // On with the show...  
    rainCounter++;
//    Serial.print("          is: ");
//    Serial.println(rainCounter);
    Serial.print("[incrementRainCount] Rain Counter : ");
    Serial.println(rainCounter);
    eeprom.writeUint16( EH_RAINFALL, rainCounter );
  }
  previousRainInterrupt = time;
}

void incrementWindCount() {
  unsigned long time = 0;
  time = millis();
  if ((time - previousWindInterrupt) > debounceInterval ) {
    windCounter++;
  }
  previousWindInterrupt = time;
}

void measureWind() {
  uint32_t startTime, endTime;
  uint16_t rotations = 0;
  uint16_t totalWindBearing = 0;
  uint8_t averageCount = 5;

/*  
  For the Davis 6410 anemometer, 1000 revolutions per hour = 1 kilometre per hour
  Our time interval is measured in milliseconds
  
     calibrationSpeed     = 1       // kilometres per hour
     calibrationRotations = 1000    // rotations per hour
     timeUnits            = 3600000 // milliseconds per hour
  
     scalingFactor = calibrationSpeed * timeUnits / calibrationRotations // For wind speed calculations
*/
 
  int scalingFactor = 3600;

  windBearing = 0;
  windSpeed = 0;

  windCounter = 0;
  Serial.println("[measureWind] Begin wind sampling...");
  attachInterrupt(windSpeedPin,incrementWindCount,FALLING);
  startTime = millis();
  
  for (int i = 0; i < averageCount; i++) {
    totalWindBearing = totalWindBearing + readWindBearing();
    delay(1000);
  }

  detachInterrupt(windSpeedPin);
  endTime = millis();
  Serial.println("[measureWind] End wind sampling");
  rotations = windCounter;
  Serial.print("[measureWind] Rotations: ");
  Serial.println(rotations);
  /*
  Serial.print("         scalingFactor : ");
  Serial.println(scalingFactor);
  Serial.print("             startTime : ");
  Serial.println(startTime);
  Serial.print("               endTime : ");
  Serial.println(endTime);
  */

  windSpeed = (int) (scalingFactor * rotations / (endTime - startTime));

  Serial.print("[measureWind] Wind Speed: ");
  Serial.print(windSpeed);
  Serial.println("kph");

  windBearing = totalWindBearing / averageCount;
  Serial.print("[measureWind] Wind Bearing: ");
  Serial.println(windBearing);
//  compassDirection(windBearing);
}

uint16_t readWindBearing() {
  //return adc level 0-4095; Need to calibrate to find max value for configuration
  uint16_t windReading = analogRead(ADC3);
//  Serial.print("[readWindBearing] Wind Reading   ");
//  Serial.println( windReading );
  uint16_t windBearing = myMap(windReading, 0, 3806, 0, 359);
  return windBearing;
}

uint16_t myMap(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
{
  if ((in_max - in_min) > (out_max - out_min)) {
    return (x - in_min) * (out_max - out_min+1) / (in_max - in_min+1) + out_min;
  }
  else
  {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }
}

void compassDirection( uint16_t windBearing ) {
//  Serial.print("[compassDirection]                 ");
  if ( windBearing > 169 ) {
    if ( windBearing > 259 ) {
      if ( windBearing > 304) {
        if ( windBearing > 326) {
          if ( windBearing > 349) {
            Serial.print("N");          
          } else { // 326 < windBearing <= 349
            Serial.print("NNW");
          }
        } else { // 304 < windBearing <= 326
          Serial.print("NW");
        }
      } else { //windBearing <= 304
        if ( windBearing > 281) {
          Serial.print("WNW");          
        } else { // 259 < windBearing <= 281
          Serial.print("W");
        }
      }
    } else { // windBearing <= 259
      if ( windBearing > 214) {
        if ( windBearing > 236) {
          Serial.print("WSW");          
        } else { // 214 < windBearing <= 236
          Serial.print("SW");
        }
      } else { //windBearing <= ?
        if ( windBearing > 191) {
          Serial.print("SSW");          
        } else { // 169 < windBearing <= 191
          Serial.print("S");
        }
      }
    }
  } else { // windBearing <= 169
    if ( windBearing > 79 ) {
      if ( windBearing > 124) {
        if ( windBearing > 146) {
          Serial.print("SSE");          
        } else { // 124 < windBearing <= 146
          Serial.print("SE");
        }
      } else { //101 < windBearing <= 124
        if ( windBearing > 101) {
          Serial.print("ESE");          
        } else { // 79 < windBearing <= 101
          Serial.print("E");
        }
      }
    } else { // windBearing <=79
      if ( windBearing > 34) {
        if ( windBearing > 56) {
          Serial.print("ENE");          
        } else { // 34 < windBearing <= 56
          Serial.print("NE");
        }
      } else { //windBearing <= 34
        if ( windBearing > 11) {
          Serial.print("NNE");          
        } else { // 349 < windBearing <= 11
          Serial.print("N");
        }
      }
    }
  }
//  Serial.println();
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

void displayRevisionLevels() {

}

void displayOff() {

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