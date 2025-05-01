/* LoRa Pump Controller Gateway Pilot
  
   Function:
   LoRa Gateway that also monitors MQTT broker, sends pump control messages as required then processes message ACKs
   
   Description:
   1. Uses packet structures defined in the PacketHandler library
  
               0.0.1  Code sample for Heltec LoRa WiFi 32 V3
               0.0.2  Add ACK management
               0.0.3  Add linked list buffer for ACK queuing
               0.0.4  Process control messages issued by the controller Node (Node-RED override)
   10 May 2023 0.0.5  Add REL bit to request ACK for any packet
   14 May 2023 0.0.6  Send 'State Undefined' message to broker if packet not acknowledged
   16 May 2023 0.0.7  Add REL bit processing for incoming packets
   18 May 2023 0.0.8  Add MQTT callback buffer
   23 May 2023 0.0.9  Add statusCheck()
   30 May 2023 0.0.10 Update readBatteryVoltage()
   03 Jun 2023 0.0.11 Rearrange main loop TX case processing options
   06 Jul 2023 0.0.12 Amend checkAckBuffer logic
   08 Jul 2023 0.0.13 Augment reportStatus function to report ACK & MQTT buffer size and content
                      Amend ackBuffer processing logic
   09 Jul 2023 0.0.14 Add serialOut() to output packets when checking ACK buffer
   25 Oct 2023 0.0.15 Add WiFi status check to Mqtt_Connect() before continuing with MQTT broker connection
   16 Dec 2023 0.0.16 Add Pump 4
   19 Dec 2023 0.0.17 Update in line with updated PUMP packet structure and functions
    4 Jan 2024 0.0.18 Update in line BPHC software 0.0.17
    8 Jan 2024 0.0.19 Amend PH_powerState entities in line with PacketHandler 0.0.21
   17 Jan 2024 0.0.20 Update to use revised PUMP packet structure (cf. PacketHandler 0.0.22)
   24 Jan 2024 0.0.21 Send MQTT message(s) from the Gateway to acknowledge the reported status of Power Switch
                      (Pump Controller) Nodes
                      Various minor amendments to tidy up the code and comments
   27 Jan 2024 0.0.22 Retain the status of individual Power Switch (Pump Controller) Nodes
   28 Feb 2024 0.0.23 Add hard reset if MQTT_Connect() fails
   10 Apr 2024 0.0.24 Correct setting of ACK packet destination MAC address
                      Various minor amendments

   Need to check behaviour with 'undefined' packet type - crashed when FLOW type was added but PacketHandler
   change not incorporated into this sketch
   
   10 Apr 2024
   Digital Concepts
   www.digitalconcepts.net.au
*/

#include <esp_int_wdt.h>        // ESP32 watchdog timers
#include <esp_task_wdt.h>

#include <Arduino.h>
#include <Wire.h>               // Need this to define I2C bus
#include <LoRaWan_APP.h>
#include <WiFi.h>               // WiFi client
#include <PubSubClient.h>       // MQTT client
#include <SSD1306.h>            // OLED display
#include <OneWire.h>            // Required for Dallas Temperature
#include <DallasTemperature.h>  // DS18B20 Library

#include <Ticker.h>             // Asynchronous task management

#include <LangloLoRa.h>         // LoRa configuration parameters
#include <LangloWiFi.h>         // [Langlo] WiFi configuration parameter
//#include <LangloMQTT.h>       // [Langlo] MQTT configuration parameters
#include <PiMQTT.h>             // [bb-7 Pi] MQTT configuration parameters
#include <PacketHandler.h>      // [Langlo] LoRa Packet management
#include <LinkedList.h>         // Linked List support for buffers
#include <WiFiLoRa32V3Pins.h>   // Heltec WiFi LoRa 32 V3 pin definitions
#include <nodeV3X1AConfig.h>    // Specific local Node configuration parameters

#include "mqttTopic.h"          // MQTT topic parser

#define wakePin           A4a_A1

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {0,0,24};
bool softRev = true;

// Timers

Ticker oledTimer, resendTimer, statusTimer;
const int oledActivationInterval = 60; // 60 seconds OLED Auto-off (Set to 0 for always on)
const int oledControlPin = displayWake;     // Hardware interrupt
bool oledActive = true;
bool oledOff = true;

const int statusReportInterval = 300; // 5 min
bool checkStatus = true;

// Reliable packet delivery stuff

const int resendInterval = 1;
const int millisResendInterval = resendInterval * 1000;
const int resendLimit = 3;
uint32_t sourceMAC = 0;
int resendCount = 0;
bool ackRequired = true;
bool sendAck = false;
bool ackReceived = false;
bool resendTimerActive = false;

struct bufferElement {
  PacketHandler packet;
  unsigned long startTime;
  int resendCount;
};
bufferElement element;
LinkedList<bufferElement> *ackBuffer = new LinkedList<bufferElement>();
int bufferSize;
bool checkAckBuffer = false;

// WiFi/MQTT

WiFiClient localClient;
PubSubClient mqttClient(localClient), *mqttClientPtr;

// We don't really need anything other than the packet in this buffer element, but using the same
// struct as the ackBuffer makes it much easier to transfer elements between buffers when required

LinkedList<bufferElement> *mqttBuffer = new LinkedList<bufferElement>();
bool checkMqttBuffer = false;

SSD1306 myDisplay(0x3c, SDA_OLED, SCL_OLED), *displayPtr;

// DS18B20
const int oneWireBus = A4a_A2;
const int temperaturePrecision = 9;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensor(&oneWire);

uint32_t messageCounter = 0;
uint32_t pumpMAC = 0;
uint8_t pumpId = 0;     // Valid values ultimately [currently] 1..4
int const pumpCount = 5;  // For the time being, the number of pumps + 1, so that we can use the pumpId as the array index
PH_powerSupply pumpPowerSupply[pumpCount] = {PH_POWER_UNDEFINED};
PH_relayState pumpRelayState[pumpCount] = {PH_RELAY_OFF};
PH_controlMode pumpControlMode[pumpCount] = {PH_MODE_UNDEFINED};
PH_packetType ackPacketType;
PacketHandler packet, inPacket, ackPacket;

static RadioEvents_t RadioEvents;
bool loraIdle = true;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( void );

typedef enum {
  RX_State,
  TX_State
} States_t;
States_t state;
int16_t rssi,rxSize;

// This function is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something

void mqttCallback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageString;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageString += (char)message[i];
  }
  Serial.println();

// If a message is received on a +/relaystate/+ topic, check if the message is either on or off.
// A 'relaystate' topic will be of the form node/type/qualifier => node/relaystate/<n>
// Send an appropriate instruction to the pump controller Node
  
  mqttTopic localTopic;
  localTopic.begin(topic);
  bool queuePacket = true;
//  String topicType = localTopic.type();
  if ( localTopic.type() == "relaystate" ) {
    pumpId = localTopic.qualifier().toInt();
    switch ( pumpId ) {
      case 1: {
        pumpMAC = pump1MAC;
        Serial.print("Pump 1 to ");
        break;
      }
      case 2: {
        pumpMAC = pump2MAC;
        Serial.print("Pump 2 to ");
        break;
      }
      case 3: {
        pumpMAC = pump3MAC;
        Serial.print("Pump 3 to ");
        break;
      }
      case 4: {
        pumpMAC = pump4MAC;
        Serial.print("Pump 4 to ");
        break;
      }
      default: {
        Serial.println("[mqttCallback] WARNING: No topic qualifier specified");
        Serial.print("               TOPIC: ");
        Serial.println(topic);
        queuePacket = false;
      }
    }
    if ( queuePacket ) {
      if ( messageString == "ON" ) {      
        pumpRelayState[pumpId] = PH_RELAY_ON;        
        Serial.print("ON");
      } else if ( messageString == "OFF" ) {      
        pumpRelayState[pumpId] = PH_RELAY_OFF;        
        Serial.print("OFF");
      } else {      
        pumpRelayState[pumpId] = PH_RELAY_UNDEFINED;        
        Serial.print("UNDEFINED");
      }
      // The sequence number will be inserted before the packet is sent  
      element.packet.begin(pumpMAC,myMAC,0,PUMP);
      element.packet.setRelFlag();
      element.packet.setPumpId(pumpId);
      element.packet.setPowerSupply(pumpPowerSupply[pumpId]);
      element.packet.setRelayState(pumpRelayState[pumpId]);
      element.packet.setControlMode(pumpControlMode[pumpId]);
      mqttBuffer->add(element);
      checkMqttBuffer = true;
      state = TX_State;
    }
  } else {
    Serial.println("[mqttCallback] WARNING: No matching topic type '" + localTopic.type() + "'");
  }
  Serial.println();
}

// Now the other functions we need to set up our environment

void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

void WiFi_Connect() {
  const int retryLimit = 10;
  int retryCount = 0;
  int retryDelay = 1000;
  Serial.println();
  Serial.println("[WiFi Connect] Attempting WiFi Connection...");
  Serial.print( "     My Name: " );
  Serial.println( myName );
  Serial.print( "        SSID: " );
  Serial.println( WiFi_SSID );
  Serial.print( "    Password: " );
  Serial.println( WiFi_Password );

  while ( retryCount < retryLimit ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi Connect] WiFi Connected");
 
      byte mac[6];
      WiFi.macAddress(mac);
      Serial.print("[WiFi Connect] MAC address: ");
      char dataString[30] = {0};
      sprintf(dataString,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
      Serial.println(dataString);
  
      Serial.print("[WiFi Connect] IP address: ");
      Serial.println(WiFi.localIP());
      retryCount = retryLimit;
    } else {
      retryCount = retryCount + 1;
      if (retryCount >= retryLimit) {
        Serial.println( "[WiFi Connect] WiFi connection attempt failed, hard restart..." );
        hard_restart();
      }
      retryDelay = retryDelay + 5000;
      WiFi.disconnect();
      delay(retryDelay);
      Serial.println( "[WiFi Connect] Retry WiFi connection..." );
      WiFi.mode(WIFI_STA);
      delay(retryDelay);
      WiFi.setHostname(myName);
      WiFi.begin(WiFi_SSID, WiFi_Password);
      delay(retryDelay);
    }
  }
}

void MQTT_Connect() {
  const int retryLimit = 10;
  int retryCount = 0;
  Serial.println();
  Serial.println("[MQTT_Connect] Start connection cycle...");

  // Check WiFi status - No point continuing without WiFi...

  if (!(WiFi.status() == WL_CONNECTED)) {
    WiFi_Connect();
  }

  while (retryCount < retryLimit) {
    retryCount = retryCount + 1;
    // Are we connected?
    if (mqttClient.connected()) {
      Serial.println("[MQTT_Connect] Connection complete" );
      Serial.println();
      // Exit loop next time around
      retryCount = retryLimit;
    } else {
      // Check the WiFi connection
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT_Connect] Attempting WiFi reconnect...");
        WiFi_Connect();
      }
      Serial.println("[MQTT_Connect] Attempting MQTT connect...");
      // OK, now try MQTT
      
      // If authentication is required, the format for the connection request is:
      //    mqttClient.connect(mqttClientId, mqttUsername, mqttPassword)
      //
      // I haven't actually tried it, but I assume that the type for all three parameters is the same:
      //    const char* mqttClientId = "gateway", mqttUsername = "fred", mqttPassword = "nurk";
            
      if (mqttClient.connect(myName)) {
        Serial.print("[MQTT_Connect] Connected to MQTT server ");
        Serial.print(MQTT_Server);
        Serial.print(":");
        Serial.print(MQTT_Port);
        Serial.println(" as " + String(myName));
        mqttClient.subscribe("node/relaystate/1");
        mqttClient.subscribe("node/relaystate/2");
        mqttClient.subscribe("node/relaystate/3");
        mqttClient.subscribe("node/relaystate/4");
      } else {
        Serial.print("[MQTT_Connect] MQTT " + String(myName) + " connect failed, rc=");
        Serial.println(mqttClient.state());
        Serial.println("[MQTT_Connect] Wait 5 seconds...");      
        delay(5000);
      }
    }
  }
  if ( !mqttClient.connected()) {
    Serial.println("[MQTT_Connect] Repeated connection failure, hard restart..." );
    hard_restart();
  }
}

void setup() {
  Serial.begin(115200);
  Mcu.begin();
  delay(500);
  Serial.print("[setup] ");
  Serial.print(myName);
  Serial.println(" Pump Controller Test");
  Serial.println();
  Serial.println("[setup] Commence Set-up...");
  
  pinMode(Builtin_LED,OUTPUT);
  pinMode(Vext_Ctrl,OUTPUT);
//  pinMode(ADC_Ctrl,OUTPUT);
//  pinMode(VBAT_Read,INPUT);
  pinMode(oledControlPin,INPUT_PULLUP);     // Activate OLED display
  pinMode(wakePin,OUTPUT);                  // Activate battery monitor

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
  myDisplay.drawString(5,5,"LoRa MQTT Monitor");
  myDisplay.display();
  delay(500);
  oledOff = false;
  displayPtr = &myDisplay;

  if ( softRev ) {
    Serial.println("[setup] softRev = true");
    Serial.println("[setup] Check display");
    displaySoftRev();
    delay(1000);
    softRev = false;
  }

  rssi=0;

//  Initialise LoRa

  Serial.println("[setup] Initialise LoRa...");
    
  // LoRa parameters defined in LangloLoRa.h
  
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
 
  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                              LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                              LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                              0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  state = TX_State;
  
  Serial.println("[setup] LoRa initialisation complete");
  myDisplay.clear();
  myDisplay.drawString(5,20,"LoRa Initialisation OK");
  myDisplay.display();

  // Connect to the WiFi network

  myDisplay.drawString(5,35,"Connecting to " + String(WiFi_SSID) + "...");
  myDisplay.display();
  WiFi_Connect();

  // Set up the MQTT client

  mqttClient.setServer(MQTT_Server, MQTT_Port);
  /* I'm not sure the keepaive is working the way it should.
   * My expectation would be that a keepalive packet was sent out periodically to let the broker know that we are still here.
   * What seems ot be happening is that everything is OK if we are transmitting packets to the broker, but if there's a period
   * roughly equal to the keepalive time that we're not transmitting, the connection is lost. So there don't seem to actually
   * be keepalive packets being transmitted (or maybe just not being acknowledged?). What appears to be the case is that the
   * connection is terminated if there is no activity within the keepalive time...
   */
  mqttClient.setKeepAlive(60);  // Default is 15 sec, but this results in frequent loss of connection
  mqttClient.setCallback(mqttCallback);

  // initialise the general Packet object
  
  inPacket.begin();

  // and set up the pointers to the OLED display and MQTT client (to pass to the Packet object when required)

  displayPtr = &myDisplay;
  mqttClientPtr = &mqttClient;
  MQTT_Connect();

  // Let the broker know there's been a reset
  
  Serial.println("[setup] Send the MQTT broker a reset message");
  messageCounter = 0;
  packet.begin(myMAC,myMAC,messageCounter,RESET);
  packet.setResetCode(0);
  packet.serialOut();
  packet.mqttOut(mqttClientPtr);
  messageCounter++;

  statusTimer.attach( statusReportInterval, setCheckStatus );

  if ( oledActivationInterval > 0 ) {
    oledTimer.once( oledActivationInterval, deactivateOLED );
    attachInterrupt( oledControlPin, activateOLED, FALLING);
  }

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {

  // Check the WiFi/MQTT connections before proceeding (the WiFi conenction can be a bit flakey)

  if (!mqttClient.connected()) {
    Serial.println("[loop] MQTT Client not connected...");
    reportMqttConnectionError();
    Serial.println("[loop] Attempting MQTT (re)connect...");   
    MQTT_Connect();
  }
  mqttClient.loop();
	switch ( state ) {
		case TX_State: {
//      Serial.println("[loop TX] Into TX Mode...");

      if ( oledActive ) {
        if ( oledOff ) {
          Serial.println();
          Serial.println("[loop TX] Turn on OLED display...");
          Serial.println();
          myDisplay.displayOn(); // Switch on display
          delay(10);
          oledOff = false;
          myDisplay.clear();
          myDisplay.setFont(ArialMT_Plain_10);
          myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
          myDisplay.drawString(5,5,"LoRa [Gateway] Receiver");
          myDisplay.display();       
          oledTimer.once( oledActivationInterval, deactivateOLED ); // Start the timer
        }
      }

      // We have four possible tasks to perform and they are intentionally ordered as they are.
      // 1. Check to see if we need to acknowledge receipt of a packet. We need to do this first because the sender
      //    is waiting for our response;
      // 2. Report on our own status, which is not time-critical;
      // 3. Send any packets that have been queued as a result of MQTT commmands.
      //    This is time-sensitive because we will be expecting an acknowledgement almost immediately. This conflicts with
      //    the similar requirements of ACK buffer processing (4.), so we can only perform one of these functions in any one cycle;
      // 4. If we have packets that require acknowledgement, and need to be resent, we send them (only one each cycle) last, because
      //    we need to transition immediately thereafter into RX_State to receive the acknowledgement.

      if ( sendAck ) {
        Serial.println("[loop TX sendAck] Send ACK...");
        inPacket.setDestinationMAC(sourceMAC);
        inPacket.setSourceMAC(myMAC);
        // Tell the broker we've seen it
        inPacket.mqttOut(mqttClientPtr);
        // and now carry on with the ACK
        inPacket.setAckFlag();
        inPacket.hexDump();
        inPacket.serialOut();
        sendMessage(inPacket);
        sendAck = false;
        // We need to wait a tick here to let the packet go out before doing anything more with the radio
        delay(100);
      }

      if ( checkStatus ) {
        statusCheck();
        checkStatus = false;
        /*
        sensorTicker.once(reportInterval, setSensorFlag);     // Come around again when the timer fires
        */
      }
      
      if ( checkMqttBuffer ) {
        // Send any packets in the MQTT buffer
        bufferElement element;
        bufferSize = mqttBuffer->size();
        Serial.print("[loop TX checkMqttBuffer] MQTT buffer size [");
        Serial.print(bufferSize);
        Serial.println("]");
        while ( bufferSize > 0 ) {
          Serial.println("[loop TX checkMqttBuffer] Retrieve next element...");
          element = mqttBuffer->remove(0);
          bufferSize = mqttBuffer->size();
          element.packet.setSequenceNumber(messageCounter);

          element.packet.hexDump();
          element.packet.serialOut();
          if ( oledActive ) element.packet.displayOut(displayPtr);
          element.packet.mqttOut(mqttClientPtr);
          sendMessage(element.packet);
          messageCounter++;
          // We need to wait a tick here to let the packet go out before doing anything more with the radio
          delay(100);

          if ( element.packet.relFlag() ) {
            // Add the packet to the buffer of packets awaiting ACK
            element.resendCount = 1;
            element.startTime = millis();
            ackBuffer->add(element);
            if ( !resendTimerActive ) {
              Serial.println("[loop TX checkMqttBuffer] Start resendTimer...");
              resendTimer.once( resendInterval, resendFunction );
              resendTimerActive = true;
            }
          }
        }
        checkMqttBuffer = false;
      } else if ( checkAckBuffer ) {
        // The ackBuffer contains copies of packets that are awaiting acknowledgement
        // If there is one, check the first element of the ACK buffer
        // We only do one resend per cycle so as not to block a response to the packet
        // that is sent
        bufferSize = ackBuffer->size();
        while ( bufferSize > 0 ) {
          Serial.print("[loop TX checkAckBuffer] ACK buffer non-empty [");
          Serial.print(bufferSize);
          Serial.println("]");

          unsigned long currentTime = millis();
          unsigned long timeInterval;

          Serial.println("[loop TX checkAckBuffer] Retrieve first element...");
          element = ackBuffer->get(0);
          timeInterval = currentTime - element.startTime;
          Serial.print("[loop TX checkAckBuffer] Current Time: ");
          Serial.println(currentTime);
          Serial.print("[loop TX checkAckBuffer] Start Time: ");
          Serial.println(element.startTime);
          Serial.print("[loop TX checkAckBuffer] Time Interval: ");
          Serial.println(timeInterval);
          Serial.print("[loop TX checkAckBuffer] Resend Interval: ");
          Serial.println(millisResendInterval);
          if (timeInterval > millisResendInterval) {
            Serial.println("[loop TX checkAckBuffer] Resend packet... ");
            resendCount = ++element.resendCount;
            Serial.print("[loop TX checkAckBuffer] Resend Count: ");
            Serial.println(resendCount);
            if ( resendCount > resendLimit ) {
              Serial.println("[loop TX checkAckBuffer] Resend Limit exceeded... ");
              Serial.println("[loop TX checkAckBuffer] Delete packet...");
              element = ackBuffer->remove(0);
              // Want to send something to the MQTT broker at this point...
              switch ( element.packet.packetType() ) {
                case PUMP: {
                  // A "State Unknown" message — turn the gateway traffic light orange
                  element.packet.setPowerSupply(PH_POWER_UNDEFINED);
                  element.packet.setRelayState(PH_RELAY_UNDEFINED);
                  element.packet.setControlMode(PH_MODE_UNDEFINED);
                  element.packet.serialOut();
                  element.packet.mqttOut(mqttClientPtr);
                  break;
                }
                // Other specific cases as required...
              }

              bufferSize = ackBuffer->size();
              Serial.print("[loop TX checkAckBuffer] New Buffer Size: ");
              Serial.println(bufferSize);
            } else {
              Serial.println("[loop TX checkAckBuffer] Move element to end of queue...");
              element = ackBuffer->remove(0);
              element.startTime = currentTime;
              element.resendCount = resendCount;
              ackBuffer->add(element);
              Serial.println("[loop TX checkAckBuffer] Resending packet...");
              element.packet.serialOut();
              sendMessage(element.packet);
              delay(100);
              // We need to go see if there's a response to this transmission, so we'll
              // pretend the buffer's empty to get us out of the buffer processing loop
              bufferSize = 0;
            }
          } else {
            bufferSize--;
          }
        }
        // We will have exited the loop after resending just one packet, so we need to
        // check to see if there's still more in the buffer that we need to come back for       
        bufferSize = ackBuffer->size();
        if ( bufferSize > 0 ) {
          if ( !resendTimerActive ) {
            Serial.println("[loop TX checkAckBuffer] Start resendTimer...");
            resendTimer.once( resendInterval, resendFunction );
            resendTimerActive = true;
          }
        } else {
          checkAckBuffer = false;
        }
      }
      
      Serial.println(); 
      state = RX_State;
      break;
    }
		case RX_State: {
      if ( loraIdle ) {
        loraIdle = false;
//        Serial.println("[loop RX] Into RX Mode...");
        Radio.Rx(0);
      }
      Radio.IrqProcess( );
/*

      // and finally an MQTT message...
  
      inPacket.mqttOut(mqttClientPtr);
      Serial.println();
      digitalWrite(LED_BUILTIN,LOW);

      // Not sure about this at this point, but...
      
      Radio.IrqProcess( );
*/
      break;
		}
    default: {
      state = RX_State;
      break;
    }
	}
//  Radio.IrqProcess( );
}

void OnTxDone( void )
{
  Serial.println("[OnTxDone] TX done!");
  Radio.Sleep( );
  loraIdle = true;    
  state = RX_State;
}

void OnTxTimeout( void )
{
  Serial.println("[OnTxTimeout] TX Timeout...");
  Radio.Sleep( );
  loraIdle = true;    
  state = RX_State;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    digitalWrite(Builtin_LED,HIGH);
    // Copy the input buffer payload to inPacket for further processing
    inPacket.setContent(payload,size);
    digitalWrite(Builtin_LED,LOW);
    Radio.Sleep( );

// Normally we'd check the destination MAC address at this point, and only continue if it was ours
    if (inPacket.destinationMAC() == myMAC) {
    
// But for testing purposes, we'll take anything
//    if (true) {
/*        
        Serial.println("[OnRxDone] This one's for us...");
*/  
      uint8_t byteCount = inPacket.packetByteCount();
/*
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
      Serial.print("             REL Flag: ");
      if ( inPacket.relFlag() ) {
        Serial.println("True");
      } else {
        Serial.println("False");
      }
      Serial.print("          Packet Type: ");
      Serial.println(inPacket.packetTypeDescription());
      Serial.print("             ACK Flag: ");
      if ( inPacket.ackFlag() ) {
        Serial.println("True");
      } else {
        Serial.println("False");
      }
      Serial.print("   Payload Byte Count: ");
      Serial.println(inPacket.payloadByteCount());
      Serial.println();
*/
      //  Verify the checksum before we go any furhter
    
      if (inPacket.verifyPayloadChecksum()) {

        Serial.println("[OnRxDone] Process the packet...");
            
        // Send relevant information to the Serial Monitor
        inPacket.hexDump();
        inPacket.serialOut();
        // print RSSI & SNR of packet
        Serial.print("[OnRxDone] RSSI ");
        Serial.print(rssi);
        Serial.print("   SNR ");
        Serial.println(snr);

        // and finally an MQTT message as required...

        if ( inPacket.ackFlag() ) {
          if ( verifyAck(inPacket) ) {
            // Reliable [ACKed] messages are currently only sent in response to direct MQTT instructions. The Gateway Node
            // acknowledges receipt of these instructions with a specific MQTT response.
            // Use the original packet (loaded into <element> during verifyAck()), which includes the payload,
            // as the basis for the MQTT response.
            element.packet.mqttOut(mqttClientPtr);
          }
        } else {
          inPacket.mqttOut(mqttClientPtr);
          // If this is a PUMP packet (currently the only packet type that uses RPDP), we need to record the status of the Controller and advise the MQTT broker
          // that we, the Gateway Node, are aware of it. To do this, we modify inPacket by setting our own MAC address as the source, and use the PacketHandler
          // mqttOut() function to generate the relevant MQTT messages. Note that inPacket is being modified in this way simply for the purpose of
          // generating appropriate MQTT messages, the packet is never actually sent anywhere.
          switch ( inPacket.packetType() ) {
            case PUMP: {
              sourceMAC = inPacket.sourceMAC();
              inPacket.setSourceMAC(inPacket.destinationMAC());
              uint8_t pumpIndex = inPacket.pumpId();
              pumpPowerSupply[pumpIndex] = inPacket.powerSupply();
              pumpRelayState[pumpIndex] = inPacket.relayState();
              pumpControlMode[pumpIndex] = inPacket.controlMode();;
              inPacket.mqttOut(mqttClientPtr);
              break;
            }
            // Other specific cases as required...
          }

          // Send relevant information to the OLED display
        
          if ( oledActive ) {
            if ( oledOff ) {
              Serial.println();
              Serial.println("[OnRxDone] Turn on OLED display...");
              Serial.println();
              myDisplay.displayOn(); // Switch on display
              oledTimer.once( oledActivationInterval, deactivateOLED ); // Start the timer
              oledOff = false;
              delay(10);
            }
            inPacket.displayOut(displayPtr);   
          // Display RSSI and SNR of packet
            myDisplay.drawHorizontalLine(0, 52, 128);
            myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
            myDisplay.drawString(0, 54, "RSSI " + String(rssi) + "dB");
            myDisplay.setTextAlignment(TEXT_ALIGN_RIGHT);
            myDisplay.drawString(128, 54, "SNR " + String(snr) +"dB");
            myDisplay.display();
          }
          if ( inPacket.relFlag() ) {
//            Serial.println("[OnRxDone check REL Flag] REL Flag set, send an ACK");
            // Send an ACK
            sendAck = true;
            state = TX_State;
          } else {
//            Serial.println("[OnRxDone check REL Flag] REL Flag not set");
          }
          Serial.println();
        }
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
/*
      Serial.print("[OnRxDone] This appears to be for someone else--To: 0x");
      Serial.print(inPacket.destinationMAC(),HEX);
      Serial.print(" From: 0x");
      Serial.println(inPacket.sourceMAC(),HEX);
*/
      inPacket.erasePacketHeader();
    }
    loraIdle = true;    
}

uint16_t readBatteryVoltage() {
// ADC resolution
  const int resolution = 12;
  const int adcMax = pow(2,resolution) - 1;
  const float adcMaxVoltage = 3.3;
// On-board voltage divider
  const int R1 = 390;
  const int R2 = 100;
// Calibration measurements
  const float measuredVoltage = 4.21;
  const float reportedVoltage = 4.11;
// Measured value multiplication factor
  const float factor = (adcMaxVoltage / adcMax) * ((R1 + R2)/(float)R2) * (measuredVoltage / reportedVoltage);
  digitalWrite(ADC_Ctrl,LOW);
  delay(100);
  int analogValue = analogRead(VBAT_Read);
  digitalWrite(ADC_Ctrl,HIGH);

  float floatVoltage = factor * analogValue;
  uint16_t voltage = (int)(floatVoltage * 1000.0);
/*
  Serial.print("[readBatteryVoltage]        ADC : ");
  Serial.println(analogValue);
  Serial.print("[readBatteryVoltage]      Float : ");
  Serial.println(floatVoltage,3);
  Serial.print("[readBatteryVoltage] milliVolts : ");
  Serial.println(voltage);
*/
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

void statusCheck() {
  Serial.println("[statusCheck] Gateway System Status");

  // Set packet pointers to local structures

  packet.setSequenceNumber(messageCounter);
  packet.setPacketType(VOLTAGE);
  packet.setVoltage(readBatteryVoltage());
  packet.serialOut();
  packet.mqttOut(mqttClientPtr);
  Serial.println();
  messageCounter++;

  packet.setSequenceNumber(messageCounter);
  packet.setPacketType(TEMPERATURE);
  packet.setTemperature(readDS18B20Sensor());
  packet.serialOut();
  packet.mqttOut(mqttClientPtr);
  Serial.println();
  messageCounter++;
  
  // Report the status of the ACK and MQTT buffers
  
  int i = 0;
  
  Serial.println("[statusCheck] ACK Buffer");
  int bufferSize = ackBuffer->size();
  Serial.print("[statusCheck] ACK buffer size: ");
  Serial.println(bufferSize);
  while ( i < bufferSize ) {
    Serial.print("[statusCheck] Buffer element: ");
    Serial.println(i);
    element = ackBuffer->get(i);
    element.packet.serialOut();
    Serial.print("[statusCheck] element.startTime: ");
    Serial.println(element.startTime);
    Serial.print("[statusCheck] element.resendCount: ");
    Serial.println(element.resendCount);
    i++;
  }
  Serial.println();
  
  Serial.println("[statusCheck] MQTT Buffer");
  bufferSize = mqttBuffer->size();
  Serial.print("[statusCheck] MQTT buffer size ");
  Serial.println(bufferSize);
  i = 0;
  while ( i < bufferSize ) {
    Serial.println("[statusCheck] Buffer element: ");
    Serial.println(i);
    element = mqttBuffer->get(i);
    element.packet.serialOut();
    i++;
  }
  Serial.println();  
}

void setCheckStatus() {
  checkStatus = true;
  state = TX_State;
}

void activateOLED() {
 /* 
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

  oledTimer.once( oledActivationInterval, deactivateOLED );
*/
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

void resendFunction() {
  Serial.println("[resendFunction] Timer fired...");
  int bufferSize = ackBuffer->size();
  if ( bufferSize > 0 ) {
    Serial.println("[resendFunction] Check Buffer...");
    checkAckBuffer = true;
    state = TX_State;
  } else {
    Serial.println("[resendFunction] Buffer empty");
    checkAckBuffer = false;
  }
  resendTimerActive = false;
}

bool verifyAck(PacketHandler ackPacket) {
  // Search the buffer for a matching outstanding ACK
  int i = 0;
  int bufferSize = ackBuffer->size();
  bool found = false;
//  Serial.print("[verifyAck] Buffer size: ");
//  Serial.println(bufferSize);
  while ( i < bufferSize && !found ) {
//    Serial.print("[verifyAck] Checking buffer element: ");
//    Serial.println(i);
    element = ackBuffer->get(i);
    ackPacketType = ackPacket.packetType();
    if ( ackPacket.sourceMAC() == element.packet.destinationMAC() &&
        ackPacket.sequenceNumber() == element.packet.sequenceNumber() && 
        ackPacketType == element.packet.packetType() ) {
      Serial.print("[verifyAck] ACK verified [");
      Serial.print(i);
      Serial.println("]");
      element = ackBuffer->remove(i);  // Crashes if we don't use the form of remove() that copies the deleted element (?)
      found = true;
    } else {
      Serial.print("[verifyAck] No match [");
      Serial.print(i);
      Serial.println("]");
      Serial.print("[verifyAck] ackPacket.sourceMAC: ");
      Serial.printf("%02X", ackPacket.sourceMAC());
      Serial.println();
      Serial.print("[verifyAck] element.packet.destinationMAC: ");
      Serial.printf("%02X", element.packet.destinationMAC());
      Serial.println();
      Serial.print("[verifyAck] ackPacket.sequenceNumber: ");
      Serial.println(ackPacket.sequenceNumber());
      Serial.print("[verifyAck] element.packet.sequenceNumber: ");
      Serial.println(element.packet.sequenceNumber());
      Serial.print("[verifyAck] element.startTime: ");
      Serial.println(element.startTime);
      Serial.print("[verifyAck] element.resendCount: ");
      Serial.println(element.resendCount);
     }
     i++;
  }
  if ( !found ) {
    Serial.println("[verifyAck] ACK verification failed");
    ackPacket.serialOut();
  }
  return found;
}

void sendMessage(PacketHandler packet) {
//  Serial.println("[sendMessage] Finished Packet");
  digitalWrite(Builtin_LED,HIGH);
  Radio.Send(packet.byteStream(), packet.packetByteCount());
  digitalWrite(Builtin_LED,LOW);
  Serial.println("[sendMessage] Packet Sent");
  Serial.println();
}

void reportMqttConnectionError() {
  switch (mqttClient.state()) {
    case -4 : {
      Serial.println("       -4 : MQTT_CONNECTION_TIMEOUT");
      break;
    }
    case -3 : {
      Serial.println("       -3 : MQTT_CONNECTION_LOST");
      break;
    }
    case -2 : {
      Serial.println("       -2 : MQTT_CONNECT_FAILED");
      break;
    }
    case -1 : {
      Serial.println("       -1 : MQTT_DISCONNECTED");
      break;
    }
    case 0 : {
      Serial.println("       0 : MQTT_CONNECTED");
      break;
    }
    case 1 : {
      Serial.println("       1 : MQTT_CONNECT_BAD_PROTOCOL");
      break;
    }
    case 2 : {
      Serial.println("       2 : MQTT_CONNECT_BAD_CLIENT_ID");
      break;
    }
    case 3 : {
      Serial.println("       3 : MQTT_CONNECT_UNAVAILABLE");
      break;
    }
    case 4 : {
      Serial.println("       4 : MQTT_CONNECT_BAD_CREDENTIALS");
      break;
    }
    case 5 : {
      Serial.println("       5 : MQTT_CONNECT_UNAUTHORIZED");
      break;
    }
  }
}

void activateSoftRev() {
  softRev = true;
}

void displaySoftRev() {
  String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);
  if ( oledOff ) {
    Serial.println();
    Serial.println("[displaySoftwareRevison] Turn on OLED display...");
    Serial.println();

    myDisplay.displayOn(); // Switch on display
    delay(10);
    oledOff = false;
  }
  Serial.println("[displaySoftwareRevison] Display software revision data...");
  myDisplay.clear();
  myDisplay.setFont(ArialMT_Plain_10);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.println("[displaySoftwareRevison] Gateway  " + sketchRev);
  myDisplay.drawString(5,6,"Gateway");
  myDisplay.drawString(60,6,sketchRev);
  Serial.println("[displaySoftwareRevison] Packet   " + inPacket.softwareRevision(PACKET_HANDLER));
  myDisplay.drawString(5,18,"Packet");
  myDisplay.drawString(60,18,inPacket.softwareRevision(PACKET_HANDLER));
  Serial.println("[displaySoftwareRevison] Node     " + inPacket.softwareRevision(NODE_HANDLER));
  myDisplay.drawString(5,30,"Node");
  myDisplay.drawString(60,30,inPacket.softwareRevision(NODE_HANDLER));
  Serial.println();
  myDisplay.display();
  softRev = false;
  if ( oledActivationInterval > 0 ) oledTimer.once( oledActivationInterval, deactivateOLED ); // Only display data until timer expires
}