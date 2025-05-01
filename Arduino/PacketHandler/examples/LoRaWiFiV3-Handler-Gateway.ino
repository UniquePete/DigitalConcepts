/* LoRa / MQTT Single Channel Gateway with System [DS18B20 and Battery] Status
 *
 * Function:
 * Test sketch - Promiscuous mode receive data (LoRa) from remote sensor nodes, with no MQTT message generation
 * 
 * Description:
 * 1. Originally developed for the [ESP32] Heltec WiFi LoRa 32 V1 dev-board, this version for the V3 board
 * 2. Uses packet structures defined in the PacketHandler library
 * 3. Individual Node details maintained and accessed through the NodeHandler library
 * 4. Output generated for Serial Monitor, OLED diaplay and/or MQTT Broker
 *
 *  1 Jun 2022 v2.0 code fully refactored, with core methods broken out into NodeHandler and PacketHandler libraries
 *  7 Sep 2022 v2.1 Flash LED when packet received
 *                  Auto-off display and activate on hardware interrupt
 * 13 Oct 2022 v3.0 Test sketch based on v2.1 'production' sketch for V3 board
 * 
 * 13 Oct 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include <esp_int_wdt.h>        // ESP32 watchdog timers
#include <esp_task_wdt.h>

#include "Arduino.h"
#include <SSD1306.h>            // OLED display
#include "LoRaWan_APP.h"        // Heltec LoRa protocol
#include <PacketHandler.h>      // [Langlo] LoRa Packet management
#include <OneWire.h>            // Required for Dallas Temperature
#include <DallasTemperature.h>  // DS18B20 Library

#include <Ticker.h>             // Asynchronous task management

#include <LangloLoRa.h>         // [Langlo] LoRa configuration parameters
//#include <ESP32Pins.h>        // ESP32-WROOM-32 pin definitions
//#include <WiFiLoRa32V2Pins.h>   // Heltec WiFi LoRa 32 V2 pin definitions
#include <WiFiLoRa32V3Pins.h>   // Heltec WiFi LoRa 32 V1 pin definitions
#include <nodeV3G1config.h>     // Specific local Node configuration parameters

#define wakePin           A4a_A1

// Instantiate the objects we're going to need

static RadioEvents_t RadioEvents;
bool lora_idle = true;

Ticker heartbeatTicker, healthCheckTicker, statusTicker, oledTicker;

const int statusCheckInterval = 600;    // 600 seconds (10 min)
const int oledActivationInterval = 30; // 300 seconds (5 min) OLED Auto-off (Set to 0 for always on)
const int oledControlPin =  displayWake;     // Hardware interrupt
bool oledActive = true;
bool oledOff = true;

// DS18B20
const int oneWireBus = A4a_A2;
const int temperaturePrecision = 9;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensor(&oneWire);

SSD1306 myDisplay(0x3c, SDA_OLED, SCL_OLED), *displayPtr;

PacketHandler inPacket, outPacket;

// Now the other functions we need to set up our environment

void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

void setup() {
  
// Get the Serial port working first
  
  Serial.begin(115200);
  Mcu.begin();
  delay(500);
  Serial.println();
  Serial.println("[setup] LoRa V3 [OLED] MQTT Gateway");
  Serial.println("[setup] Commence Set-up...");
  
  pinMode(Builtin_LED,OUTPUT);
  pinMode(oledControlPin,INPUT_PULLUP);        // Activate OLED display
  pinMode(wakePin,OUTPUT);        // Activate battery monitor
  digitalWrite(wakePin,HIGH);

// Initialise the OLED display
  
  Serial.println("[setup] Initialise display...");
  pinMode(RST_OLED,OUTPUT);     // GPIO16
  digitalWrite(RST_OLED,LOW);   // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  myDisplay.init();
  myDisplay.clear();
//  myDisplay.flipScreenVertically();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  myDisplay.drawString(5,5,"LoRa [Gateway] Receiver");
  myDisplay.display();
  oledOff = false;

//  Initialise LoRa

  Serial.println("[setup] Initialise LoRa...");
  Serial.println("[setup] Activate LoRa Receiver");
    
  // LoRa parameters defined in LangloLoRa.h

  RadioEvents.RxDone = OnRxDone;
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                              LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                              LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                              0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  
  Serial.println("[setup] LoRa Initialisation OK");
  myDisplay.drawString(5,20,"LoRa Initialisation OK");
  myDisplay.display();

  // initialise the Packet object instances
  
  inPacket.begin();
  outPacket.begin(myMAC,myMAC,0); // This is just to report our own status from time to time

  // and set up the pointers to the OLED display and MQTT client (to pass to the Packet object when required)

  displayPtr = &myDisplay;

  // Let the broker know there's been a reset
  
  outPacket.setPacketType(RESET);
  outPacket.setResetCode(0);
  outPacket.serialOut();

  // the DS18B20 Temperature Sensor

  ds18b20Sensor.begin();

  // ADC control

  pinMode(A4a_A1,OUTPUT);

  // and finally the timers

  heartbeatTicker.attach( inPacket.heartbeatInterval(), heartbeat );
  healthCheckTicker.attach( inPacket.healthCheckInterval(), healthCheck );
  statusTicker.attach( statusCheckInterval, statusCheck );
  if ( oledActivationInterval > 0 ) {
    oledTicker.once( oledActivationInterval, deactivateOLED );
    attachInterrupt( oledControlPin, activateOLED, FALLING);
  }

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {
  
//  Now just loop around checking to see if there's anything to process

  if ( oledActive ) {
    if ( oledOff ) {
      Serial.println();
      Serial.println("[loop] Turn on OLED display...");
      Serial.println();
      myDisplay.displayOn(); // Switch on display
      delay(10);
      oledOff = false;
      myDisplay.clear();
      myDisplay.setFont(ArialMT_Plain_10);
      myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
      myDisplay.drawString(5,5,"LoRa [Gateway] Receiver");
      myDisplay.display();       
      oledTicker.once( oledActivationInterval, deactivateOLED ); // Start the timer
    }
  }

  if( lora_idle ) {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    inPacket.setContent(payload,size);
    Radio.Sleep( );

// Normally we'd check the desitnation MAC address at this point, and only continue if it was ours
// But for testing purposes, we'll take anything

    if (true) {
/*        
        Serial.println("[OnRxDone] This one's for us...");
*/  
      uint8_t byteCount = inPacket.packetByteCount();

      Serial.println("[OnRxDone] Packet received");
      Serial.println("[OnRxDone] Header content:");
      Serial.print("       DestinationMAC: ");
      Serial.printf("%02X", inPacket.destinationMAC());
      Serial.println();
      Serial.print("            SourceMAC: ");
      Serial.printf("%02X", inPacket.sourceMAC());
      Serial.println();
      Serial.print("           Sequence #: ");
      Serial.println(inPacket.sequenceNumber());
      Serial.print("          Packet Type: ");
      Serial.printf("%02X", inPacket.packetType());
      Serial.println();
      Serial.print("   Payload Byte Count: ");
      Serial.println(inPacket.payloadByteCount());
      Serial.println();

      //  Verify the checksum before we go any furhter
    
      if (inPacket.verifyPayloadChecksum()) {

        digitalWrite(Builtin_LED,HIGH);
        Serial.println("[OnRxDone] Process the packet...");
            
        // Send relevant information to the Serial Monitor
        inPacket.hexDump();
        inPacket.serialOut();
        // print RSSI & SNR of packet
        Serial.print("[OnRxDone] RSSI ");
        Serial.print(rssi);
        Serial.print("   SNR ");
        Serial.println(snr);

        // Send relevant information to the OLED display
       
        if ( oledActive ) {
          inPacket.displayOut(displayPtr);   
        // print RSSI and SNR of packet
          myDisplay.drawHorizontalLine(0, 52, 128);
          myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
          myDisplay.drawString(0, 54, "RSSI " + String(rssi) + "dB");
          myDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
          myDisplay.drawString(128, 54, "SNR " + String(snr) +"dB");
          myDisplay.display();
        }
      
        Serial.println();
        digitalWrite(Builtin_LED,LOW);
        
      } else {
        Serial.println("[OnRxDone] Invalid checksum, packet discarded");
        Serial.println();
        inPacket.erasePacketHeader();
       }
/*
          Serial.println("[receivePacket] Checksum validation disabled");
          Serial.println();
          return true;
*/
    } else {
      
      // No need to go any further

      Serial.print("[OnRxDone] This appears to be for someone else--To: 0x");
      Serial.print(inPacket.destinationMAC(),HEX);
      Serial.print(" From: 0x");
      Serial.println(inPacket.sourceMAC(),HEX);
      inPacket.erasePacketHeader();
    }
    lora_idle = true;    
}

void heartbeat() {
  inPacket.incrementNodeTimers();
}

void healthCheck() {
  Serial.println("[healthCheck] Reporting Health Check results");
  Serial.println();
}

void statusCheck() {
  Serial.println("[statusCheck] Gateway System Status");

  // Set packet pointers to local structures

  outPacket.setPacketType(VOLTAGE);
  outPacket.setVoltage(readBatteryVoltage());
  outPacket.serialOut();

  outPacket.setPacketType(TEMPERATURE);
  outPacket.setTemperature(readDS18B20Sensor());
  outPacket.serialOut();
  Serial.println();

//  readBatteryVoltage(); // Note: This is for V1 board. V2 boards have internal circuitry, but note also that this varies with the V2 rev level!
//  sendMessage(voltageTypePacket, messageCounter);
}

void activateOLED() {
  
  Serial.println();
  Serial.println("[activateOLED] Interrupt intercepted... activating OLED");
  Serial.println();
  
  myDisplay.displayOn(); // Switch on display
  
  digitalWrite(RST_OLED,LOW);  // set GPIO16 LOW to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  myDisplay.init();
  myDisplay.clear();
//  myDisplay.flipScreenVertically();
  myDisplay.setFont(ArialMT_Plain_10);

  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  myDisplay.drawString(5,5,"LoRa [Gateway] Receiver");
  myDisplay.display();

  oledTicker.once( oledActivationInterval, deactivateOLED );

  oledActive = true;
}

void deactivateOLED() {
  Serial.println();
  Serial.println("[deactivateOLED] Timer expired... deactivating OLED");
  Serial.println();

  myDisplay.displayOff(); // Switch off display
  oledActive = false;
  oledOff = true;
}

uint16_t readBatteryVoltage() {
  static const float voltageFactor = (3.2/4096)*(133.0/100)*(4.14/4.20); // (max. ADC voltage/ADC range) * voltage divider factor * calibration factor
  uint16_t inputValue = 0, peakValue = 0;
//  Serial.println("[readBatteryLevel] Read ADC...");
  
  digitalWrite(A4a_A1,HIGH);
  delay(900);
  for (int i = 0; i < 10; i++) {
    delay(10);
    inputValue = analogRead(A4a_A0); // and read ADC (Note: 33kΩ/100kΩ voltage divider in monitoring circuit)
//    Serial.print(String(inputValue) + " = ");
//    Serial.println(String(inputValue*voltageFactor) + "V");
    if (inputValue > peakValue) peakValue = inputValue;
  }
//  Serial.print(String(peakValue) + " = ");
//  Serial.println(String(peakValue*voltageFactor) + "V");
//  Serial.println();
//  digitalWrite(A4a_A1,LOW);
  
  uint16_t voltage = (int)(peakValue * voltageFactor * 1000);

  return voltage;
}

int16_t readDS18B20Sensor() {
  int16_t temperature = 0;
//  Serial.println("[readDS18B20Sensor] Read sensor...");
  ds18b20Sensor.requestTemperatures(); // Send the command to get temperatures
  
  // After we get the temperature(s), we can print it/them here.
  // We use the function ByIndex and get the temperature from the first sensor (there's only one at the moment).
//  Serial.println("[readDS18B20Sensor] Device 1 (index 0)");
  float sensorValue = ds18b20Sensor.getTempCByIndex(0);
//  Serial.print("[readDS18B20Sensor] Returned value: ");
//  Serial.println(sensorValue);

  temperature = (int) (10*(sensorValue + 0.05));  // °C x 10
//  Serial.print("[readDS18B20Sensor] Temperature: ");
//  Serial.println((float) temperature/10, 1);

  return temperature;
}
