/* LoRa Sender Node
 *
 * Function:
 * Assemble packets using the PacketHandler and forward data to the nominated Gateway Node
 * 
 * Description:
 * 1. Uses packet structures defined in the PacketHandler library
 * 2. Output generated for Serial Monitor and/or OLED diaplay
 *
 * 7 Oct 2022 v1.0 Code sample for CubeCell Dev Board Plus
 * 
 * 7 Oct 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include "LoRa_APP.h"
#include "Arduino.h"
#include <HT_SH1107Wire.h>     // CubeCell Plus display library
#include <PacketHandler.h>    // [Langlo] LoRa Packet management

#include "LangloLoRa.h"       // LoRa configuration parameters
#include "nodeP1Dconfig.h"     // Node-specific configuration parameters

/*
 * set LoraWan_RGB to 1,the RGB active in loraWan
 * RGB red means sending;
 * RGB green means received done;
 */
#ifndef LoraWan_RGB
#define LoraWan_RGB 0
#endif

// The CubeCell Plus Display object
SH1107Wire  display(0x3c, 500000, SDA, SCL, GEOMETRY_128_64, GPIO10), *displayPtr; // addr, freq, sda, scl, resolution, rst

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );

// LowPower stuff
// const int reportInterval = 60000;   // Report values every 60000 milliseconds (60 seconds)

#define wakeTime 1000
#define sleepTime 60000
static TimerEvent_t sleep;
static TimerEvent_t wakeUp;

typedef enum
{
    LOWPOWER,
    ReadVoltage,
    TX
} States_t;

States_t state;
bool sleepMode = false;
int16_t rssi,rxSize;
uint16_t batteryVoltage;

// Instantiate the Packet Handler
PacketHandler packet;

// Sensor value variables
uint8_t humidity = 0;
uint16_t pressure = 0;
uint16_t temperature = 0;

uint16_t windBearing = 0;
uint8_t windSpeed = 0;

const float LSB = 0.02761647; // per manual calibration (when using int in 1/10mm)
uint8_t inputRainfall = 0;
int adc0 = 0;

uint16_t messageCounter = 0;

void onSleep()
{
  Serial.printf("[onSleep] Go into LowPower mode for %d ms\r\n",sleepTime);
  state = LOWPOWER;
  TimerSetValue( &wakeUp, sleepTime );
  TimerStart( &wakeUp );
}

void onWake()
{
  Serial.println(""); 
  Serial.printf("[onWake] Wake for %d ms, then go back into LowPower mode.\r\n",wakeTime);
  state = ReadVoltage;
  TimerSetValue( &sleep, wakeTime );
  TimerStart( &sleep );
}

void setup() {
  boardInitMcu( );
  Serial.begin(115200);
  delay(500);                         //Let things settle down...
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" LoRa/Display Test");
  Serial.println();

  batteryVoltage=0;
  rssi=0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

  Radio.Sleep( );

  Serial.println("[setup] LoRa initialisation complete");
  Serial.println();
  
  TimerInit( &sleep, onSleep );
  TimerInit( &wakeUp, onWake );
  onWake();
  
  displayPtr = &display;
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.screenRotate(ANGLE_180_DEGREE);
  display.clear();
  
  // initialise the Packet Handler
  
  packet.begin(gatewayMAC,myMAC);

  Serial.println("[setup] Set-up Complete");
  Serial.println(""); 
}

void loop()
{
	switch(state)
	{
		case TX:
    {
      Serial.println("[loop] Send Messages");

      // Populate the packet
    
      packet.setSequenceNumber(messageCounter);
      packet.setPacketType(VOLTAGE);

      packet.setVoltage(batteryVoltage);
      packet.hexDump();
      packet.serialOut();
      packet.displayOut(displayPtr);  
      
      turnOnRGB(COLOR_SEND,0);
      sendMessage();
      turnOffRGB();
      
      // Have a snooze while the gateway hadndles that one
      delay(100);  //LowPower time
      
      packet.setPacketType(WIND);
      readSensor();
      packet.setWindBearing(windBearing);
      packet.setWindSpeed(windSpeed);
      packet.hexDump();
      packet.serialOut();   
            
      turnOnRGB(COLOR_SEND,0);
      sendMessage();
      turnOffRGB();
      
      Serial.println("[loop] Increment Message Counter");
      messageCounter++;                 // Increment message count
      Serial.println(""); 
      state=LOWPOWER;
      break;
    }
		case LOWPOWER:
		{
      lowPowerHandler();
      break;
		}
    case ReadVoltage:
    {
      batteryVoltage=getBatteryVoltage();
      Serial.print("[readSensor] Battery Voltage: ");
      Serial.print( batteryVoltage );
      Serial.println(" mV");
/*      
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(10, 5, "Voltage:");
      display.setFont(ArialMT_Plain_16);
      String voltageString = String(batteryVoltage);
      voltageString.concat(" mV");
      display.drawString(10, 20, voltageString);
      display.display();
*/
      state = TX;
      break;
    }
    default:
        break;
	}
//    Radio.IrqProcess( );
}

void OnTxDone( void )
{
  Serial.println("[OnTxDone] TX done!");
  turnOnRGB(0,0);
  state=LOWPOWER;
}

void OnTxTimeout( void )
{
  Radio.Sleep( );
  Serial.println("[OnTxTimeout] TX Timeout...");
  state=LOWPOWER;
}

void readSensor() {
/*
 * IF this node is reading atmospheric conditions from a BME sensor
 * 
  temperature = (int) (10*bme280.getTemperature());
  pressure = (int) (bme280.getPressure() / 91.79F);
  humidity = (int) (bme280.getHumidity());

  Serial.println("");
  Serial.print("[readSensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  Serial.print("[readSensor] Pressure: ");
  Serial.println(pressure);
  Serial.print("[readSensor] Humidity: ");
  Serial.println(humidity);
*/

/*
 * When we've actually got a sensor to read, this variable should be set to the value
 * read from the sensor
 *
//  inputRainfall = (int) adc0 * LSB;
  inputRainfall = 155; // Dummy value (in 1/10 mm) just so taht there's somethign other than 0 to report

  Serial.print("[readSensor] Rainfall: ");
  Serial.println((float) inputRainfall/10, 1);
*/

/*
 * When we've actually got a sensor to read, these variables should be set to the values
 * read from the sensor(s)
 */
  windBearing = 270;
  windSpeed = 10;
/*
  Serial.print("[readSensor] Wind Direction: ");
  Serial.println(windBearing);
  Serial.print("[readSensor] Wind Speed: ");
  Serial.println(windSpeed);
*/

//  batteryVoltage = getBatteryVoltage();
//  Serial.print("[readSensor] Battery Voltage: ");
//  Serial.print( getBatteryVoltage() );
//  Serial.println(" mV");

}

void sendMessage() {
  Serial.println("[sendMessage] Finished Packet");
  Radio.Send((uint8_t *)packet.byteStream(), packet.packetByteCount());
  Serial.println("[sendMessage] Packet Sent");
  Serial.println();
}
