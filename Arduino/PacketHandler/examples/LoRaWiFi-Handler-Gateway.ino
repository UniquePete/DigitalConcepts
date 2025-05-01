/* LoRa / MQTT Single Channel Gateway with System [DS18B20 and Battery] Status
 *
 * Function:
 * Receive data forwarded, via LoRa, from remote sensor nodes and formulate and forward MQTT messages, via WiFi, to the local MQTT Broker.
 * 
 * Description:
 * 1. Originally developed for the [ESP32] Heltec WiFi LoRa 32 V1 dev-board
 * 2. Uses packet structures defined in the PacketHandler library
 * 3. Individual Node details maintained and accessed through the NodeHandler library
 * 4. Output generated for Serial Monitor, OLED diaplay and/or MQTT Broker
 *
 *  1 Jun 2022 v2.0 code fully refactored, with core methods borken out into NodeHandler and PAcketHandler libraries
 *  7 Sep 2022 v2.1 Flash LED when packet received
 *                  Auto-off display and activate on hardware interrupt (Requires 10068-BHWL 1.4 or later, or 10068-BHWL3)
 * 19 Jan 2023 v2.2 Use interrupt to display software rev levels (Requires 10068-BHWL 1.4 or later, or 10068-BHWL3)
 * 
 * 1 Jun 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include <esp_int_wdt.h>        // ESP32 watchdog timers
#include <esp_task_wdt.h>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           

#include <SSD1306.h>            // OLED display
#include <SPI.h>                // LoRa radio interface
#include <LoRa.h>               // LoRa protocol
#include <WiFi.h>               // WiFi client
#include <PubSubClient.h>       // MQTT client
#include <PacketHandler.h>      // [Langlo] LoRa Packet management
#include <OneWire.h>            // Required for Dallas Temperature
#include <DallasTemperature.h>  // DS18B20 Library

#include <Ticker.h>             // Asynchronous task management

#include <LangloLoRa.h>         // [Langlo] LoRa configuration parameters
#include <LangloWiFi.h>         // [Langlo] WiFi configuration parameter
//#include <LangloMQTT.h>       // [Langlo] MQTT configuration parameters
#include <PiMQTT.h>             // [bb-7 Pi] MQTT configuration parameters
//#include <WiFiLoRa32V1Pins.h> // Heltec WiFi LoRa 32 V1 pin definitions
//#include <nodeX2config.h>     // Specific local Node configuration parameters
#include <WiFiLoRa32V2Pins.h>   // Heltec WiFi LoRa 32 V2 pin definitions
//#include <WiFiLoRa32V3Pins.h>   // Heltec WiFi LoRa 32 V3 pin definitions
#include <nodeV2G1config.h>     // Specific local Node configuration parameters
//#include <nodeV3G1config.h>     // Specific local Node configuration parameters
//#include <ESP32Pins.h>        // ESP32-WROOM-32 pin definitions
//#include <nodeX3config.h>     // Specific local Node configuration parameters

#define wakePin           A4a_A1

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {2,2,0};
bool softRev = true;

// Instantiate the objects we're going to need

Ticker heartbeatTicker, healthCheckTicker, statusTicker, oledTicker;

const int statusCheckInterval = 600;    // 600 seconds (10 min)
const int oledActivationInterval = 30; // 30 seconds OLED Auto-off (Set to 0 for always on)
const int oledControlPin =  displayWake;     // OLED Wake hardware interrupt
bool oledActive = true;
bool oledOff = true;

// DS18B20
const int oneWireBus = A4a_A2;
const int temperaturePrecision = 9;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensor(&oneWire);

WiFiClient localClient;
PubSubClient mqttClient(localClient), *mqttClientPtr;

SSD1306 display(0x3c, SDA_OLED, SCL_OLED), *displayPtr;

PacketHandler inPacket, outPacket;

// Now the other functions we need to set up our environment

// This function is executed when this MCU receives a message with a topic to which it has subscribed.
// Change the function below to add logic to your program, so that when a message is received you can
// actually do something. 

void mqttCallback(String topic, byte* message, unsigned int length) {
  Serial.print("[mqttCallback] Message arrived on Topic: ");
  Serial.print(topic);
  Serial.print("  Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

}

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
    //      mqttClient.subscribe("inTopic");
      } else {
        Serial.print("[MQTT_Connect] MQTT " + String(myName) + " connect failed, rc=");
        Serial.println(mqttClient.state());
        Serial.println("[MQTT_Connect] Wait 5 seconds...");      
        delay(5000);
      }
    }
  }
}

void setup() {
  
// Get the Serial port working first
  
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("[setup] LoRa [OLED] MQTT Gateway");
  Serial.println("[setup] Commence Set-up...");
  
  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(oledControlPin,INPUT_PULLUP); // Activate OLED display
  pinMode(triggerPin,INPUT_PULLUP);     // Activate software revision display
  pinMode(wakePin,OUTPUT);              // Activate battery monitor
  digitalWrite(wakePin,HIGH);

// Initialise the OLED display
  
  Serial.println("[setup] Initialise display...");
  pinMode(RST_OLED,OUTPUT);     // GPIO16
  digitalWrite(RST_OLED,LOW);  // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  display.init();
  display.clear();
//  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5,5,"LoRa [Gateway] Receiver");
  display.display();
  oledOff = false;

//  Initialise LoRa

  Serial.println("[setup] Initialise LoRa...");
  SPI.begin(SCK,MISO,MOSI,CS);
  LoRa.setPins(CS,RST,DIO0);
  Serial.println("[setup] Activate LoRa Receiver");
    
  // LoRa parameters defined in LangloLoRa.h
  
  if (!LoRa.begin(Frequency)) {
    display.drawString(5,25,"Starting LoRa failed!");
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
  
  Serial.println("[setup] LoRa Initialisation OK");
  display.drawString(5,20,"LoRa Initialisation OK");
  display.display();

  // Connect to the WiFi network

  display.drawString(5,35,"Connecting to " + String(WiFi_SSID) + "...");
  display.display();
  WiFi_Connect();

  // Set up the MQTT client

  mqttClient.setServer(MQTT_Server, MQTT_Port);
  /* I'm not sure the keepaive is working the way it should.
   * My expectation would be that a keepalive packet was sent out periodically to let the broekr know that we are still here.
   * What seems ot be happening is that everything is OK if we are transmitting packets to the broker, but if there's a period
   * roughly equal to the keepalive time that we're not transmitting, the conneciton is lost. So there don't seem to actually
   * be keepalive packets being transmitted (or maybe just not being acknowledged?). What appears to be the case is that the
   * connection is terminated if there is no activity within the keepalive time...
   */
  mqttClient.setKeepAlive(60);  // Default is 15 sec, but this results in frequent loss of connection
  mqttClient.setCallback(mqttCallback);

  // initialise the Packet object instances
  
  inPacket.begin();
  outPacket.begin(myMAC,myMAC,0); // This is just to report our own status from time to time

  // and set up the pointers to the OLED display and MQTT client (to pass to the Packet object when required)

  displayPtr = &display;
  mqttClientPtr = &mqttClient;
  MQTT_Connect();

  // Let the broker know there's been a reset
  
  outPacket.setPacketType(RESET);
  outPacket.setResetCode(0);
  outPacket.serialOut();
  outPacket.mqttOut(mqttClientPtr);

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
  attachInterrupt( triggerPin, activateSoftRev, FALLING);

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {

  // Check the WiFi/MQTT connections before proceeding (the WiFi conenction can be a bit flakey)

  if (!mqttClient.connected()) {
    Serial.println("[loop] MQTT Client not connected...");
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
    Serial.println("[loop] Attempting MQTT (re)connect...");   
    MQTT_Connect();
//    mqttClient.loop();
  }
  
//  Now just loop around checking to see if there's anything to process
  
  if ( oledActive ) {
    if ( oledOff ) {
        Serial.println();
        Serial.println("[loop] Turn on OLED display...");
        Serial.println();
        display.displayOn(); // Switch on display
        delay(10);
        oledOff = false;
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(5,5,"LoRa [Gateway] Receiver");
        display.display();       
        oledTicker.once( oledActivationInterval, deactivateOLED ); // Start the timer
    }
  }

  if ( softRev ) {
    Serial.println("[loop] softRev = true");
    Serial.println("[loop] Check display");
    displaySoftRev();
    softRev = false;
  }
  
  if ( receivePacket() ) {
    digitalWrite(LED_BUILTIN,HIGH);
    Serial.println("[loop] Process the packet...");
         
    // Send relevant information to the Serial Monitor
    inPacket.hexDump();
    inPacket.serialOut();
    // print RSSI & SNR of packet
    Serial.print("[loop] RSSI ");
    Serial.print(LoRa.packetRssi());
    Serial.print("   SNR ");
    Serial.println(LoRa.packetSnr());

    // Send relevant information to the OLED display
    
    if ( oledActive ) {
      inPacket.displayOut(displayPtr);   
     // print RSSI and SNR of packet
      display.drawHorizontalLine(0, 52, 128);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(0, 54, "RSSI " + (String)LoRa.packetRssi() + "dB");
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(128, 54, "SNR " + (String)LoRa.packetSnr() +"dB");
      display.display();
    }
  
    // and finally an MQTT message...
    
    inPacket.mqttOut(mqttClientPtr);
    Serial.println();
    digitalWrite(LED_BUILTIN,LOW);
  }
}

bool receivePacket() {
  
// Only if there's something there

  if ( LoRa.parsePacket() ) {
    while ( LoRa.available() ) {
      LoRa.beginPacket();
    
//    Read the packet header
//    The destination address first, to make sure it's for us

      int headerLimit = inPacket.headerSize();
      uint8_t nextByte;
      
      for (int i = 0; i < headerLimit; i++) {
        nextByte = LoRa.read();
        inPacket.setByte(i,nextByte);
        Serial.printf( "%02X", nextByte);
        if ((i % 4) == 3) {
          Serial.println("    ");
        } else {
          Serial.print("  ");
        }

      }
      if (inPacket.destinationMAC() == myMAC) {

//      Read in the rest of the packet
/*        
        Serial.println("[receivePacket] This one's for us...");

        uint32_t sourceMAC = be32toh(incomingPacket.content.sourceMAC);
        int sequenceNumber = (int) be16toh(incomingPacket.content.sequenceNumber);
        uint8_t packetType = incomingPacket.content.type;
*/  
        uint8_t byteCount = inPacket.packetByteCount();

        Serial.println("[receivePacket] Packet received");
        Serial.println("[receivePacket] Header content:");
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
        Serial.println("[receivePacket] Payload bounds:");
        Serial.print("   Start (headerSize): ");
        Serial.println(headerLimit);
        Serial.print("      End (byteCount): ");
        Serial.println(byteCount);
        Serial.println();

//        Serial.println("[receivePacket] Packet payload...");   

        for (int i = headerLimit; i < byteCount; i++) {
          inPacket.setByte(i,LoRa.read());
/*
          Serial.printf( "%02X", incomingPacket.content.packetPayload.payload[i]);
          if ((i % 4) == 3) {
            Serial.println("    ");
          } else {
            Serial.print("  ");
          }
*/
        }
//        if ((byteCount % 4) != 0) Serial.println("");
       
        //  Verify the checksum before we go any furhter
     
        if (inPacket.verifyPayloadChecksum()) {

          // All good
          
          return true;
        } else {
          Serial.println("[receivePacket] Invalid checksum, packet discarded");
          Serial.println();
          inPacket.erasePacketHeader();
          return false;
        }
/*
          Serial.println("[receivePacket] Checksum validation disabled");
          Serial.println();
          return true;
*/
      } else {
        
        // No need to go any further

        Serial.print("[receivePacket] This appears to be for someone else--To: 0x");
        Serial.print(inPacket.destinationMAC(),HEX);
        Serial.print(" From: 0x");
        Serial.println(inPacket.sourceMAC(),HEX);
        inPacket.erasePacketHeader();
        return false; 
      }

    }
  } else {
    
//  Nothing there

    return false;     
  }
}

void heartbeat() {
  inPacket.incrementNodeTimers();
}

void healthCheck() {
  inPacket.nodeHealthCheck(mqttClientPtr);
}

void statusCheck() {
  Serial.println("[statusCheck] Gateway System Status");

  // Set packet pointers to local structures

  outPacket.setPacketType(VOLTAGE);
  outPacket.setVoltage(readBatteryVoltage());
  outPacket.serialOut();
  outPacket.mqttOut(mqttClientPtr);

  outPacket.setPacketType(TEMPERATURE);
  outPacket.setTemperature(readDS18B20Sensor());
  outPacket.serialOut();
  outPacket.mqttOut(mqttClientPtr);
  Serial.println();

//  readBatteryVoltage(); // Note: This is for V1 board. V2 boards have internal circuitry, but note also that this varies with the V2 rev level!
//  sendMessage(voltageTypePacket, messageCounter);
}

void activateOLED() {
/*  
  Serial.println();
  Serial.println("[activateOLED] Interrupt intercepted... activating OLED");
  Serial.println();
  
  display.displayOn(); // Switch on display
  
  digitalWrite(RST_OLED,LOW);  // set GPIO16 LOW to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  display.init();
  display.clear();
//  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5,5,"LoRa [Gateway] Receiver");
  display.display();

  oledTicker.once( oledActivationInterval, deactivateOLED );
*/
  oledActive = true;
}

void deactivateOLED() {
  Serial.println();
  Serial.println("[deactivateOLED] Timer expired... deactivating OLED");
  Serial.println();

  display.displayOff(); // Switch off display
  oledActive = false;
  softRev = false;
  oledOff = true;
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

    display.displayOn(); // Switch on display
    delay(10);
    oledOff = false;
  }
  Serial.println("[displaySoftwareRevison] Display software revision data...");
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.println("[displaySoftwareRevison] Gateway  " + sketchRev);
  display.drawString(5,6,"Gateway");
  display.drawString(60,6,sketchRev);
  Serial.println("[displaySoftwareRevison] Packet   " + inPacket.softwareRevision(PACKET_HANDLER));
  display.drawString(5,18,"Packet");
  display.drawString(60,18,inPacket.softwareRevision(PACKET_HANDLER));
  Serial.println("[displaySoftwareRevison] Node     " + inPacket.softwareRevision(NODE_HANDLER));
  display.drawString(5,30,"Node");
  display.drawString(60,30,inPacket.softwareRevision(NODE_HANDLER));
  Serial.println("[displaySoftwareRevison] EEPROM   " + inPacket.softwareRevision(EEPROM_HANDLER));
  display.drawString(5,42,"EEPROM");
  display.drawString(60,42,inPacket.softwareRevision(EEPROM_HANDLER));
  Serial.println();
  display.display();
  softRev = false;
  if ( oledActivationInterval > 0 ) oledTicker.once( oledActivationInterval, deactivateOLED ); // Only display data until timer expires
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
