/* Basic LoRa PacketHandler framework for CubeCell
 *
 * Function:
 * Read sensors and forward data to LoRa Gateway
 * 
 * Description:
 * 1. Core framework for CubeCell platform
 * 2. Uses Langlo PacketHandler for message formatting
 * 3. Includes battery voltage and BME sensor
 *
 * Rev 1.0  2 Jul 2022
 * Rev 2.0  26 Aug 2022
 *  1. Add EEPROM for Sequence #
 *          12 Dec 2022
 *  2. Tidy up comments
 * 
 * Digital Concepts
 * digitalconcepts.net.au
 *
 */

#include <LoRa_APP.h>
#include <Arduino.h>
#include <AT24C32N.h>          // EEPROM
#include <Seeed_BME280.h>
#include <Wire.h>

#include "LangloLoRa.h"       // LoRa configuration parameters
#include "PacketHandler.h"    // Packet structures
#include "CubeCellPins.h"     // CubeCell pin definitions
//#include "nodeW1config.h"    // CubeCell 1 Node-specific configuration parameters
//#include "nodeW2config.h"    // CubeCell 2 Node-specific configuration parameters
//#include "nodeW3config.h"    // CubeCell 3 Node-specific configuration parameters
#include "nodeW4config.h"    // CubeCell 3 Node-specific configuration parameters

const uint8_t softwareRevMajor = 2;
const uint8_t softwareRevMinor = 0;

EEPROM_AT24C32N bhcEEPROM;
#define I2C_EEPROM_Address  0x50
#define sequenceLocation     0
#define sequenceBytes 2

union EEPROMPayload {
  char payloadByte[2];
  uint16_t counter;
};

EEPROMPayload EEPROMWriteData, EEPROMReadData;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

// LowPower stuff
// const int reportInterval = 60000;   // Report values every 60000 milliseconds (60 seconds)

#define wakeTime 5000
#define sleepTime 60000
static TimerEvent_t sleep;
static TimerEvent_t wakeUp;

typedef enum
{
    LOWPOWER,
    TX
} States_t;

States_t state;

int16_t rssi,rxSize;
uint16_t batteryVoltage;

// Instantiate the Packet Handler
PacketHandler packet;

// Sensor value variables
uint16_t humidity = 0;
uint16_t pressure = 0;
uint16_t temperature = 0;

uint16_t windBearing = 0;
uint8_t windSpeed = 0;

const float LSB = 0.02761647;   // per manual calibration (when using int in 1/10mm)
uint16_t inputRainfall = 0;
int adc0 = 0;

uint16_t messageCounter = 0;

BME280 bme280;

void onSleep()
{
  Serial.printf("[onSleep] Go into LowPower mode for %d ms\r\n",sleepTime);
  state = LOWPOWER;
  digitalWrite(Vext, HIGH);    // Turn off the external power before sleeping
  TimerSetValue( &wakeUp, sleepTime );
  TimerStart( &wakeUp );
}

void onWake()
{
  Serial.println(); 
  Serial.printf("[onWake] Wake for %d ms, then go back into LowPower mode.\r\n",wakeTime);
  state = TX;
  TimerSetValue( &sleep, wakeTime );
}

void setup() {
  boardInitMcu( );
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW);     // Turn on the external power supply
  Serial.begin(115200);
  delay(200);                 // Let things settle down before we try to send anything out...
  
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" CubeCell Handler Sender");
  Serial.println("[setup] BME Sketch Rev: " + String(softwareRevMajor + "." + softwareRevMinor));
  Serial.println("[setup] PacketHandler Rev: " + packet.softwareRevision());
  Serial.println();


  batteryVoltage=0;
  rssi=0;

  RadioEvents.TxDone = onTxDone;
  RadioEvents.TxTimeout = onTxTimeout;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

  Radio.Sleep( );
    
  Serial.println("[setup] LoRa initialisation complete");
  
  TimerInit( &sleep, onSleep );
  TimerInit( &wakeUp, onWake );
  onWake();

  // initialise the Packet Handler
  
  packet.begin(gatewayMAC,myMAC);

  // Let the gateway/broker know there's been a reset
  
  packet.setPacketType(RESET);
  packet.setResetCode(0);
  packet.serialOut();
  turnOnRGB(COLOR_SEND,0);
  sendMessage();
  turnOffRGB();

  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop()
{
 // Serial.println("[loop] Enter Loop");
	switch(state) {
		case TX: {
      Serial.print("[loop-TX] Read Sequence Number       ");
      digitalWrite(Vext, LOW);    // Turn on the external power supply
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();               // On with the show...
      bhcEEPROM.RetrieveBytes(EEPROMReadData.payloadByte, sequenceBytes, sequenceLocation, false);
      Wire.end();
      Serial.println(EEPROMReadData.counter);
      messageCounter = EEPROMReadData.counter + 1;

      batteryVoltage=getBatteryVoltage();
      Serial.print("[loop-TX] Battery Voltage: ");
      Serial.print( batteryVoltage );
      Serial.println(" mV");
      
      packet.setSequenceNumber(messageCounter);
      packet.setPacketType(VOLTAGE);
      packet.setVoltage(batteryVoltage);
      packet.hexDump();
      packet.serialOut();

      Serial.println("[loop-TX] Send Voltage Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage();
      turnOffRGB();
      
      // Have a snooze while the gateway hadndles that one
      delay(100);
      
      packet.setPacketType(ATMOSPHERE);
      readBMESensor();
      packet.hexDump();
      packet.serialOut();   
        
      Serial.println("[loop-TX] Send Atmosphere Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage();
      turnOffRGB();
      
      digitalWrite(Vext, LOW);    // Make sure the external power supply is on
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();               // On with the show...
      EEPROMWriteData.counter = messageCounter;
      bhcEEPROM.WriteBytes(sequenceLocation, sequenceBytes, EEPROMWriteData.payloadByte);
      Wire.end();                 // All done here
      digitalWrite(Vext, HIGH);   // Turn off the external power supply
      
      Serial.println(""); 
      state=LOWPOWER;

      TimerStart( &sleep );

	    break;
    }
		case LOWPOWER:
    {
      lowPowerHandler();
      break;
    }
    default:
        break;
	}
//    Radio.IrqProcess( );
}

void onTxDone( void )
{
  Serial.println("[OnTxDone] TX done!");
  Radio.Sleep( );
}

void onTxTimeout( void )
{
  Serial.println("[OnTxTimeout] TX Timeout...");
  Radio.Sleep( );
}

void readBMESensor() {
  Serial.println("[readSensor] Read sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/
  digitalWrite(Vext, LOW);    // Turn on the external power supply
  delay(50);                  // Give everything a moment to settle down
  Wire.begin();               // On with the show...
  
  if (bme280.init(0x76)) {
    Serial.println( "[readSensor] BME sensor [0x76] initialisation at complete" );
  } else if (bme280.init(0x77)) {
    Serial.println( "[readSensor] BME sensor [0x77] initialisation at complete" );
  } else {
    Serial.println( "[readSensor] BME sensor initialisation error" );
  }

  delay(100); // The BME280 needs a moment to get itself together... (50ms is too little time)

  temperature = (int) (10*bme280.getTemperature());
  pressure = (int) (bme280.getPressure() / 91.79F);
  humidity = (int) (bme280.getHumidity());

  packet.setTemperature(temperature);
  packet.setPressure(pressure);
  packet.setHumidity(humidity);

  Wire.end();

  Serial.println("");
  Serial.print("[readSensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readSensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readSensor] Humidity: ");
  Serial.println(humidity);
}

void sendMessage() {

  int totalByteCount = packet.packetByteCount();
  
  Serial.println("[sendMessage] Finished Packet");
  Radio.Send((uint8_t *)packet.byteStream(), packet.packetByteCount());
  Serial.println("[sendMessage] Packet Sent");
  Serial.println();
}
