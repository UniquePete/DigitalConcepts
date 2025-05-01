/* CubeCell LoRa Power Switch
  
   Function:
   LoRa/PacketHandler Power Switch with Local Override
   
   Description:
   1. Uses packet structures defined in the PacketHandler library
   2. Sends battery status, DS18B20 and BME280 sensor data as well as recognising Power Switch Control commands
   3. Includes Local Override for local switch control
   4. Supports external TPL5010 watchdog timer
  
    6 May 2023 0.0.4  Packet Echo with ACK Flag
   16 May 2023 0.0.5  REL flag with interrupt for local override
               0.0.6  Add REL/ACK buffer
   22 May 2023 0.0.7  TX_State code rearrangement
   30 May 2023 0.0.8  Remove reference to EEPROM Handler
    1 Jun 2023 0.0.9  General housekeeping to get the various implementations [more or less] consistent
    3 Jun 2023 0.0.10 Report pump status with sensor readings
   10 Jun 2023 0.0.11 Add watchdog timer
   22 Jun 2023 0.0.12 Correct pump state change logic
    6 Jul 2023 0.0.13 Correct checkAckBuffer logic
    9 Jul 2023 0.0.14 Randomise reportingPeriod to avoid Node 'update syncing'
   23 Aug 2023 0.0.15 Update code to run on fully configured 9575-BPHC v2.1 PCB
   16 Dec 2023 0.0.16 Add EEPROM management (EepromHandler) code
                      (EEPROM must be initialised before running this sketch)
                      Add TPL5010 watchdog timer code
    4 Jan 2024 0.0.17 Add local override code
                      Move TPL5010 watchdog timer code to sendMessage()
    6 Jan 2024 0.0.18 Add code to read DS18B20 and report Node temperature
    6 Jan 2024 0.0.19 Remove pump status report from setup and start loop with sensor read
    6 Jan 2024 0.0.20 Move all initial EEPROM access to the beginning of the setup() function
                      (All but the DESCRIPTOR retrieval was previously grouped with the packet initialisation)
    8 Jan 2024 0.0.21 Amend PH_powerState entities in line with PacketHandler 0.0.21
                      Add code to read ACS712 current sensor. At this point, we're only using this to determine
                      whether or not there is an active external power supply, so that we can raise an alarm if
                      there is a power failure.
    8 Jan 2024 0.0.22 Add debug code to try to determine when the EEPROM (GM field) is getting overwritten
                      Add serial monitor message for LIVE power (checkPowerSupply())
   16 Jan 2024 0.0.23 Update to use revised PUMP packet structure (cf. PacketHandler 0.0.22)
   27 Jan 2024 0.0.24 Send a pump status packet after ACKing an update request
   31 Jan 2024 0.0.25 Remove debug code (cf. 0.0.22)
                      Add code to only conditionally use BME280
    9 Feb 2024 0.0.26 Change name of ISR function setControlMode() to changeControlMode() to avoid any possibility
                      of conflict or confusion with the PacketHandler function of the same name
    9 Apr 2024 0.0.27 Rework interrupt processing and Node status reporting
   17 Apr 2024 0.0.28 Further rework interrupt processing
                      Implement direct calls to NeoPixel library for NeoPixel control
   28 Apr 2024 0.0.29 Zero pressInterruptTime and releaseInterruptTime when resetting interruptFlag
   18 May 2024 0.0.30 Still trying to sort out spurious interrupt problem...
                      Declare ISR shared variables as volatile
                      Use digitalPinToInterrupt(toggleButton), rather than just toggleButton, to define interrupt
                      'Invert' interrupt definitions to reflect button configuration change to 'pull to ground'
    3 Jun 2024 0.0.31 Set localControlLEDOn flag consistently when changing LED state
    5 Jun 2024 0.0.32 Add more comments
                      Simplify neoPixel colour specification mechanism
    5 Jan 2025 0.0.33 Initialise radio state in setup()
                      Anything else?
                       
   ?? ??? 202? 0.0.1? Add code to enter Local Control Mode on startup and switch to Remote Control Mode
                      only after establishing communications with the Gateway (maybe flash the Local Mode
                      LED until communications have been established), or maybe after some time limit if
                      communications cannot be established
   
   5 Jan 2025
   Digital Concepts
   www.digitalconcepts.net.au
 */

#include <Arduino.h>
#include <Wire.h>               // I2C Bus
#include <CubeCell_NeoPixel.h>  // CubeCell NeoPixel Library
#include <LoRa_APP.h>           // Heltec LoRa Library
#include <OneWire.h>            // DS18B20 Bus
#include <DallasTemperature.h>  // DS18B20 Library
#include <Seeed_BME280.h>

#include <LangloLoRa.h>         // LoRa configuration parameters
#include <EepromHandler.h>      // [Langlo] EEPROM management
#include <PacketHandler.h>      // [Langlo] LoRa Packet management
#include <LinkedList.h>         // Linked List support for REL/ACK buffer
#include <CubeCellV2Pins.h>     // Heltec CubeCell V2 pin definitions

#define wdtDone          GPIO0  // Watchdog Timer Done [Output]
#define oneWireBus       GPIO1  // A4a_A2 – DS18B20 [I/O]
#define localControlLED  GPIO2  // Local Control LED [Output]
#define toggleButton     GPIO3  // Local/Remote Control & Pump ON/OFF Control Toggle Button [Input]
#define relayControl     GPIO5  // Relay ON/OFF Control [Output]

#define bme280Present    false  // Set to true if BME280 configured

struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};

softwareRevision sketchRevision = {0,0,33};
bool softRev = true;

// NeoPixel Parameters: # pixels, RGB or RGBW device, device colour order + frequency
CubeCell_NeoPixel neo(1, RGB, NEO_GRB + NEO_KHZ800);

// EEPROM object and variables
EepromHandler eeprom;
EH_dataType parameter;
uint32_t gatewayMAC, myMAC;
uint8_t* myName;
uint16_t messageCounter = 0;
uint8_t pumpId = 0;

// The reporting period is randomised to help avoid syncing of Node updates
// Even if near synchronous updates from different Nodes don't collide,
// the Gateway cannot process multiple packets in quick succession
const int lowerReportingLimit = 55000; // 55 sec
const int upperReportingLimit = 65000; // 65 sec
int reportingPeriod; // Will be randomised between lowerReportingLimit and upperReportingLimit

// Watchdog timer variables
const int wdtInterval = 60;         // seconds
unsigned long wdtLimit = 180000;    // milliseconds
unsigned long lastMessageTime = 0;

// RPDP variables
const int resendInterval = 1;                       // seconds
const int eventCycleTime = resendInterval * 1000;   // milliseconds
const int resendLimit = 3;
int resendCount = 0;
bool ackReceived = false;
bool resendTimerActive = false;

// Miscellaneous flags
bool pumpTimerActive = false;
bool powerFailure = false;
bool reportNodeStatus = true;
bool ackRequired = true;
bool sendAck = false;
bool readSensors = true;
bool changePumpState = false;
bool reportControlMode = false;
bool localControlLEDOn = false;

// Commentary on the subject suggests that variables used within an ISR should be declared as volatile
volatile uint32_t pressInterruptTime = 0;
volatile uint32_t releaseInterruptTime = 0;
volatile bool interruptFlag = false;

// Interrup variables
const uint32_t overrideTriggerInterval = 3000;    // milliseconds
const uint32_t debounceInterval = 100;            // milliseconds

// LED [flash] variables
const uint32_t flashInterval = 200;              // milliseconds
const uint8_t flashCount = 6;                    // # times LED will turn ON + OFF when triggered
uint8_t flashCounter = 0;

typedef enum {
  NP_OFF,
  NP_RED,
  NP_GREEN,
  NP_BLUE
} neoPixelColour_t;

// Packet element types
PH_powerSupply nodePowerSupply = PH_POWER_LIVE;
PH_relayState relayState = PH_RELAY_OFF;
PH_controlMode relayControlMode = PH_MODE_REMOTE;
PH_packetType ackPacketType, messageType;
PacketHandler inPacket, outPacket;

// Packet buffer
struct bufferElement {
  PacketHandler packet;
  unsigned long startTime;
  int resendCount;
};
bufferElement element;
LinkedList<bufferElement> *ackBuffer = new LinkedList<bufferElement>();
int bufferSize;
bool checkAckBuffer = false;

// Sensor variables
// DS18B20
const int temperaturePrecision = 9;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature ds18b20Sensor(&oneWire);

#if bme280Present
  // BME280

  uint16_t humidity = 0;
  uint16_t pressure = 0;
  uint16_t temperature = 0;
  BME280 bme280;
#endif

// Timers
static TimerEvent_t resendTimer;
static TimerEvent_t sensorTimer;
static TimerEvent_t pumpWatchDogTimer;
static TimerEvent_t overrideTimer;
static TimerEvent_t flashTimer;

static RadioEvents_t RadioEvents;
bool lora_idle = true;
void OnTxDone(void);
void OnTxTimeout(void);
void OnRxDone(void);

typedef enum {
  RX_State,
  TX_State
} States_t;

States_t state;
int16_t rssi,rxSize;

void setup() {            
  boardInitMcu();
  reportingPeriod = random(lowerReportingLimit, upperReportingLimit);
  Serial.begin(115200);
  delay(500);
  
  Serial.println("[setup] Commence Set-up...");

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);     // Turn the external power supply ON

// Retrieve Node details from EEPROM
  
  Wire.begin();
  eeprom.begin(&Wire);
  gatewayMAC = eeprom.readUint32(EH_GATEWAY_MAC);
  myMAC = eeprom.readUint32(EH_NODE_MAC);
  myName = eeprom.readBytes(EH_DESCRIPTOR);
  int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
  Serial.print("[setup] ");
  if ( myName[0] == 0 ) {
    Serial.print("No Name");
  } else {
    for (int i = 0; i < byteCount; i++) {
      Serial.print((char)myName[i]);
    }
  }
  Serial.println(" Prototype");
  messageCounter = eeprom.readUint16(EH_SEQUENCE) + 1;
  pumpId = eeprom.readUint8(EH_PUMPID);
  
  if ( softRev ) {
    printSoftRev();
    softRev = false;
  }

  pinMode( wdtDone, OUTPUT );
  digitalWrite(wdtDone, LOW);      // TPL5010 reset

  pinMode(relayControl, OUTPUT);
  digitalWrite(relayControl, LOW);  // Turn the pump OFF

/*
 Toggle button interrupt
 The initial press triggers an interrupt on the RISING edge and sets a new
 interrupt on the FALLING edge. The latter measures the time interval between
 the RISE and FALL and re-establishes the RISING edge interrupt.
 */

//  PINMODE_INPUT_PULLDOWN( toggleButton );
//  attachInterrupt( digitalPinToInterrupt(toggleButton), toggleButtonPressed, RISING );
  PINMODE_INPUT_PULLUP(toggleButton);
  attachInterrupt(digitalPinToInterrupt(toggleButton), toggleButtonPressed, FALLING);

  pinMode(localControlLED,OUTPUT);

//  Initialise LoRa

  Serial.println("[setup] Initialise LoRa...");

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  state = TX_State;
  Radio.Sleep();
  rssi=0;

  Serial.println("[setup] LoRa initialisation complete");
  Serial.println();

// Initialise the Packet object instances
  
  inPacket.begin();
  outPacket.begin(gatewayMAC, myMAC, messageCounter);

/*
Initialise timers

The Sensor Read timer triggers the periodic reading of sensors
*/
  TimerInit(&sensorTimer, setSensorFlag);
  Serial.println("[setup] Sensor read timer started...");
  TimerSetValue(&sensorTimer, reportingPeriod);
  TimerStart(&sensorTimer);
/*
The ACK Buffer timer sets the retransmission interval for packets
that require acknowledgement. If a 'reliable' packet is not acknowledged
in <eventCycleTime> it is retransmitted.
*/
  TimerInit(&resendTimer, resendFunction);
  Serial.println("[setup] ACK timer initialised");
  TimerSetValue(&resendTimer, eventCycleTime);
/*
The Pump Watchdog timer is used to monitor the pump power supply. The controller
must receive confirmation of the pump power state at least every <wdtLimit> milliseconds
or the pump will be turned OFF.
*/
  TimerInit(&pumpWatchDogTimer, switchOffRelay);
  Serial.println("[setup] Pump timer initialised");
  TimerSetValue(&pumpWatchDogTimer, wdtLimit);
/*
The Override timer is used to trigger the flashing of the Mode LED to signal that the Mode button
can be released.
*/
  TimerInit(&overrideTimer, flashLed);
  Serial.println("[setup] Local override timer initialised");
  TimerSetValue(&overrideTimer, overrideTriggerInterval);
/*
The LED flasher timer is used to signal that the Mode Button has been pressed and held for
the overrideTriggerInterval. Releasing the Mode Button will now toggle the Node operating Mode.
*/
  TimerInit( &flashTimer, flashLed );
  Serial.println("[setup] Flash timer initialised");
  TimerSetValue(&flashTimer, flashInterval);
 
  // Let the broker know there's been a reset
  
  outPacket.setPacketType(RESET);
  outPacket.setResetCode(0);
  outPacket.serialOut();
  sendMessage(outPacket);
  // We need to wait a tick here to let the packet go out before doing anything more with the radio or the packet gets lost
  delay(100);
  messageCounter++;

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {
  checkInterruptStatus();
	switch ( state ) {
		case TX_State: {
//      Serial.println("[loop TX] Into TX Mode...");

      // We have four possible tasks to perform
      // 1. Check to see if we need to acknowledge receipt of a packet. We need to do this first because the sender
      //    is waiting for our response;
      // 2. If we have sensor data to send, we need to send that;
      // 3. If a local pump state change has been triggered, we need to advertise the fact to the Gateway/MQTT broker.
      //    This is time-sensitive because we will be expecting an acknowledgement almost immediately. This conflicts with
      //    the similar requirements of ACK buffer processing (4.), so we can only perform one of these functions in any one cycle;
      // 4. If we have packets that require acknowledgement, and need to be resent, we send them (only one each cycle) last, because
      //    we need to transition immediately thereafter into RX_State to receive the acknowledgement.

      if ( sendAck ) {
        Serial.println("[loop TX sendAck] Begin...");
        inPacket.setDestinationMAC(inPacket.sourceMAC());
        inPacket.setSourceMAC(myMAC);
        inPacket.setAckFlag();
        inPacket.hexDump();
        inPacket.serialOut();
        sendMessage(inPacket);
        sendAck = false;
        // We need to wait a tick here to let the packet go out before doing anything more with the radio
        delay(100);
        // If that was a PUMP packet, we also need to confirm our current status
        switch ( inPacket.packetType() ) {
          case PUMP: {
            outPacket.setSequenceNumber(messageCounter);
            recordPumpStatus();
            outPacket.hexDump();
            outPacket.serialOut();
            sendMessage(outPacket);
            messageCounter++;
            // We need to wait a tick here to let the packet go out before doing anything more with the radio or the packet gets lost
            delay(100);
            break;
          }
            // Other specific cases as required...
        }
      }

      if ( readSensors ) {
        Serial.println("[loop TX readSensors] Begin...");
        outPacket.setSequenceNumber(messageCounter);
        readBatteryVoltage();
        outPacket.hexDump();
        outPacket.serialOut();
        sendMessage(outPacket);
        messageCounter++;                 // Increment message count
        // We need to wait a tick here to let the packet go out before continuing for two reasons:
        // 1. For some reason, if we're too quick to continue, the packet doesn't go out
        // 2. The Gateway can't handle two packets in quick succession
        delay(100);

        checkPowerSupply();

        outPacket.setSequenceNumber(messageCounter);
        readDS18B20Sensor();
        outPacket.hexDump();
        outPacket.serialOut();
        sendMessage(outPacket);
        messageCounter++;
        // We need to wait a tick here again to let the packet go out before doing anything more with the radio or the packet gets lost
        delay(100);

        #if bme280Present
          outPacket.setSequenceNumber(messageCounter);
          readBMESensor();
          outPacket.hexDump();
          outPacket.serialOut();
          sendMessage(outPacket);
          messageCounter++;                 // Increment message count
          // We need to wait a tick here again to let the packet go out before doing anything more with the radio or the packet gets lost
          delay(100);
        #endif

        // Report pump status
        outPacket.setSequenceNumber(messageCounter);
        recordPumpStatus();
        outPacket.hexDump();
        outPacket.serialOut();
        sendMessage(outPacket);
        messageCounter++;                 // Increment message count
        // We need to wait a tick here to let the packet go out before doing anything more with the radio or the packet gets lost
        delay(100);

        readSensors = false;
        TimerStart(&sensorTimer);     // Come around again when the timer fires
      }

      if ( reportNodeStatus ) {
        Serial.println("[loop TX reportNodeStatus] Begin...");
        /*
        Since this packet will require acknowledgement, we assemble it in the ACK
        buffer from the outset
        */
        bufferElement element;
        element.packet.begin(gatewayMAC, myMAC, messageCounter, PUMP);
        element.packet.setRelFlag();
        element.packet.setPumpId(pumpId);
        /*
        if ( relayState == PH_RELAY_ON ) {
          Serial.println("[loop TX reportNodeStatus] relayState : ON");
        } else {
          Serial.println("[loop TX reportNodeStatus] relayState : OFF");
        }
        */
        element.packet.setPowerSupply(nodePowerSupply);
        element.packet.setRelayState(relayState);
        element.packet.setControlMode(relayControlMode);
        element.packet.serialOut();
        sendMessage(element.packet);
        messageCounter++;                 // Increment message count
        if ( !resendTimerActive ) {
          Serial.println("[loop TX reportNodeStatus] Start resendTimer...");
          TimerStart( &resendTimer );
          resendTimerActive = true;
        }
        // Add the packet to the buffer of packets awaiting ACK
        element.resendCount = 1;
        element.startTime = millis();
        ackBuffer->add(element);
        reportNodeStatus = false;
      } else if ( checkAckBuffer ) {
        /*
        Check the ACK buffer for any packets that need to be retransmitted (i.e. for which we
        have not received acknowledgement)
        */
        Serial.println("[loop TX checkAckBuffer] Begin...");
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
          Serial.println(eventCycleTime);
          if ( timeInterval > eventCycleTime ) {
            Serial.println("[loop TX checkAckBuffer] Resend packet... ");
            resendCount = ++element.resendCount;
            Serial.print("[loop TX checkAckBuffer] Resend count: ");
            Serial.println(resendCount);
            if ( resendCount > resendLimit ) {
              Serial.println("[loop TX checkAckBuffer] Resend Limit exceeded... ");
              Serial.println("[loop TX checkAckBuffer] Delete packet...");
              element = ackBuffer->remove(0);
              // May want to do something more at this point...

              bufferSize = ackBuffer->size();
              Serial.print("[loop TX checkAckBuffer] New Buffer Size: ");
              Serial.println(bufferSize);
            } else {
              // Move the element to the end of the queue
              Serial.print("[loop TX checkAckBuffer] Resend count: ");
              Serial.println(resendCount);
              Serial.println("[loop TX checkAckBuffer] Move element to end of queue...");
              element = ackBuffer->remove(0);
              element.startTime = currentTime;
              element.resendCount = resendCount;
              ackBuffer->add(element);
              Serial.println("[loop TX checkAckBuffer] Resending packet...");
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
            TimerStart( &resendTimer );
            resendTimerActive = true;
            checkAckBuffer = false;
          }
        } else {
          checkAckBuffer = false;
        }
        Serial.println("[loop TX checkAckBuffer] End");
        Serial.println();
      }

//      Serial.println(); 
      state=RX_State;
      break;
    }
		case RX_State: {
//      Serial.println("[loop RX] Into RX Mode...");
      if ( lora_idle ) {
        lora_idle = false;
        Radio.Rx(0);
      }
      Radio.IrqProcess( );
      state = TX_State;
      break;
		}
    default: {
      state = RX_State;
      break;
    }
	}
}

void OnTxDone(void) {
//  Serial.println("[OnTxDone] TX done!");
  Radio.Sleep();
  lora_idle = true;    
  state = RX_State;
}

void OnTxTimeout(void) {
//  Serial.println("[OnTxTimeout] TX Timeout...");
  Radio.Sleep();
  lora_idle = true;    
  state = RX_State;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  neoPixel(NP_RED);   // NeoPixel [low intensity] RED
  inPacket.setContent( payload, size );
  neoPixel(NP_OFF);    // NeoPixel OFF
  Radio.Sleep();
  lora_idle = true;    

  if (inPacket.destinationMAC() == myMAC) {
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
    //  Verify the checksum before we go any further
  
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

      // and finally, respond when required...

      if ( inPacket.ackFlag() ) {
        if ( verifyAck(inPacket) ) {
          // Nothing more to do here at the moment
        }
      } else {
        switch (inPacket.packetType()) {
          case PUMP: {
            if ( relayControlMode == PH_MODE_LOCAL ) {
              reportNodeStatus= true;
            } else {
              relayState = inPacket.relayState();
              if ( relayState == PH_RELAY_ON ) {
                digitalWrite(relayControl, HIGH);
                lastMessageTime = millis();
                if ( pumpTimerActive ) {
                  TimerReset(&pumpWatchDogTimer);
                } else {
                  TimerStart(&pumpWatchDogTimer);
                  pumpTimerActive = true;
                }
              } else {
                digitalWrite(relayControl, LOW);
                if ( pumpTimerActive ) {
                  TimerStop (&pumpWatchDogTimer);
                }
              }
            }
            if ( inPacket.relFlag() ) {
              sendAck = true;
              state = TX_State;
            }
            break;
          }
          default: {
            Serial.println("[OnRxDone packetType Switch] Well, what am I supposed to do with this...?");
          }
        }
      }
  
      Serial.println();
      
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
}

void sendMessage(PacketHandler packet) {
  digitalWrite(Vext, LOW);    // Make sure EEPROM power is ON
  Wire.begin();               // and that the I2C bus is active
  eeprom.writeUint16(EH_SEQUENCE, messageCounter);
  neoPixel(NP_GREEN);   // NeoPixel [low intensity] GREEN
  Radio.Send(packet.byteStream(), packet.packetByteCount());
  neoPixel(NP_OFF);    // NeoPixel OFF
//  Serial.println("[sendMessage] Packet Sent");
//  Serial.println("[sendMessage] Pat the dog...");
  digitalWrite(wdtDone, HIGH);
  digitalWrite(wdtDone, LOW);
  Wire.end();
  digitalWrite(Vext, HIGH);    // Turn off the external power supply
  Serial.println();
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
      Serial.println("[verifyAck] ACK verified");
      element = ackBuffer->remove(i);  // Crashes if we don't use the form of remove() that copies the deleted element (?)
      found = true;
    } /* else {
      Serial.println("[verifyAck] No match");
      Serial.print("[verifyAck] ackPacket.sourceMAC(): ");
      Serial.println(ackPacket.sourceMAC(),HEX);
      Serial.print("[verifyAck] element.packet.destinationMAC(): ");
      Serial.println(element.packet.destinationMAC(),HEX);
      Serial.print("[verifyAck] ackPacket.sequenceNumber(): ");
      Serial.println(ackPacket.sequenceNumber());
      Serial.print("[verifyAck] element.packet.sequenceNumber(): ");
      Serial.println(element.packet.sequenceNumber());
    } */
    i++;
  }
  if ( !found ) {
    Serial.println("[verifyAck] ACK verification failed");
    ackPacket.serialOut();
  }
  return found;
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

void setSensorFlag() {
  Serial.println("[setSensorFlag] Timer fired...");
  readSensors = true;
  state = TX_State;
}

void switchOffRelay() {
  relayState = PH_RELAY_OFF;
  digitalWrite(relayControl, LOW);
  Serial.println("[switchOffRelay] Relay OFF");
  pumpTimerActive = false;
  state = TX_State;
}

void toggleButtonPressed() {
  Serial.println("[toggleButtonPressed] FALLING Interrupt...");
  pressInterruptTime = millis();
  releaseInterruptTime = 0;
  flashCounter = flashCount;
  TimerStart(&overrideTimer);     // Signal user when button release will activate local override
  // Set interrupt to catch button release
  attachInterrupt(toggleButton, toggleButtonReleased, RISING);
}

void toggleButtonReleased() {
  Serial.println("[toggleButtonReleased] RISING Interrupt...");
  releaseInterruptTime = millis();
  TimerStop(&overrideTimer);
  TimerStop(&flashTimer);
  // Set interrupt to catch next button press
  attachInterrupt(toggleButton, toggleButtonPressed, FALLING);
  // We only need to act on interrupts after the button has been released
  interruptFlag = true;
}

void flashLed() {
  Serial.println("[flashLed] Timer fired...");
  if ( localControlLEDOn ) {
    digitalWrite(localControlLED, LOW);
    localControlLEDOn = false;
  } else {
    digitalWrite(localControlLED, HIGH);
    localControlLEDOn = true;
  }
  flashCounter--;
  if ( flashCounter > 0 ) {
    TimerStart( &flashTimer );      // Come around again when the timer fires
  } else {                          // Check the active Mode and leave the Mode LED in the appropriate state
    if ( relayControlMode == PH_MODE_REMOTE ) {
      digitalWrite(localControlLED, LOW);     // Turn Local LED OFF
      localControlLEDOn = false;
    } else {
      digitalWrite(localControlLED, HIGH);     // Turn Local LED ON
      localControlLEDOn = true;
    }
  }
}

void checkInterruptStatus() {
  if ( interruptFlag ) {
    Serial.println("[checkInterruptStatus] Process interrupt...");
    if (( releaseInterruptTime - pressInterruptTime ) > overrideTriggerInterval ) {
      Serial.println("[checkInterruptStatus] Override trigger...");
      toggleControlMode();
    } else if ( releaseInterruptTime > ( pressInterruptTime + debounceInterval )) {
      Serial.println("[checkInterruptStatus] Check mode...");
      if ( relayControlMode == PH_MODE_LOCAL ) {
        toggleRelayState();
      }
    }
    // Reset the interrupt variables
    interruptFlag = false;
    pressInterruptTime = 0;
    releaseInterruptTime = 0;
  }
}

void toggleControlMode() {
  if ( relayControlMode == PH_MODE_REMOTE ) {
    relayControlMode = PH_MODE_LOCAL;
    digitalWrite(localControlLED, HIGH);     // Turn Local LED ON
    localControlLEDOn = true;
    Serial.println("[toggleControlMode] Local Control Mode");
  } else {
    relayControlMode = PH_MODE_REMOTE;
    Serial.println("[toggleControlMode] Remote Control Mode");
    digitalWrite(localControlLED, LOW);     // Turn Local LED OFF
    localControlLEDOn = false;
  }
  reportNodeStatus = true;
}

void toggleRelayState() {
  if ( relayState == PH_RELAY_ON ) {
    relayState = PH_RELAY_OFF;
    digitalWrite(relayControl, LOW);
    Serial.println("[toggleRelayState] Relay OFF");
  } else {
    relayState = PH_RELAY_ON;
    digitalWrite(relayControl, HIGH);
    Serial.println("[toggleRelayState] Relay ON");
  }
  reportNodeStatus = true;
}

void readBatteryVoltage() {
  uint16_t batteryVoltage = getBatteryVoltage();
  Serial.print("[readBatteryVoltage] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
  outPacket.setPacketType(VOLTAGE);
  outPacket.setVoltage(batteryVoltage);
}

void checkPowerSupply() {
  uint16_t voltage = getVoltageLevel();
  Serial.print("[checkPowerSupply] Power Source: ");
  if (voltage == 0) {
    Serial.println("FAIL");
    nodePowerSupply = PH_POWER_FAIL;
  } else {
    Serial.println("LIVE");
    nodePowerSupply = PH_POWER_LIVE;
  }
  Serial.println();
}

uint16_t getVoltageLevel() {
/*
  Use the ACS712 current sensor reading to determine whether or not it is actually receiving power.
  If there's no 'external' power, the sensor will not be active and will return a zero value.
  If there is external power, the sensor will be powered and there will be a small residual
  voltage (10-15mV), whether or not the power switch is ON.

  This is the same basic process that we'd use to measure power consumption when the Node switch is ON,
  so this operation could simply be included in the power consumption mearuement process when we do
  get around to implementing that.
*/
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here (will be reset to the max reading taken)
  int minValue = 4096;          // store min value here - CubeCell ADC resolution - (will be reset to the min reading taken)
  float result;
  uint16_t millivolts;

  uint32_t start_time = millis();
  //sample for 1 Sec
  while((millis()-start_time) < 1000) {
    readValue = analogRead(ADC);
    // see if you have a new maxValue
    if (readValue > maxValue) {
        /*record the maximum sensor value*/
        maxValue = readValue;
    }
    if (readValue < minValue) {
        /*record the minimum sensor value*/
        minValue = readValue;
    }
  } 
  // Subtract min from max
  result = ((maxValue - minValue) * 3.3)/4096.0; //ASR650x ADC resolution 4096
  millivolts = (uint16_t)(1000 * result);
  return millivolts;
}

void readDS18B20Sensor() {
  digitalWrite(Vext, LOW);    // Make sure the external power supply is on
  delay(10);
  ds18b20Sensor.begin();
  delay(100);                 // Give everything a moment to settle down
//  Serial.println("[readDS18B20Sensor] Read sensor...");
  ds18b20Sensor.requestTemperatures(); // Send the command to get temperatures
  // After we get the temperature(s), we can print it/them here.
  // We use the function ByIndex and get the temperature from the first sensor (there's only one at the moment).
//  Serial.println("[readDS18B20Sensor] Device 1 (index 0)");
  float sensorValue = ds18b20Sensor.getTempCByIndex(0);
//  Serial.print("[readDS18B20Sensor] Returned value: ");
//  Serial.println(sensorValue);
  digitalWrite(Vext, HIGH);    // Turn off the external power supply
  int16_t temperature = (int) (10*(sensorValue + 0.05));  // °C x 10
  Serial.print("[readDS18B20Sensor] Temperature: ");
  Serial.println((float) temperature/10, 1);
  outPacket.setPacketType(TEMPERATURE);
  outPacket.setTemperature(temperature);
}

#if bme280Present
  void readBMESensor() {
    Serial.println("[readBMESensor] Read sensor...");
  /*
  * If this node is reading atmospheric conditions from a BME sensor
  * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
  */
    digitalWrite(Vext, LOW);    // Turn on the external power supply
    delay(50);                  // Give everything a moment to settle down
    Wire.begin();               // On with the show...
    
    if (bme280.init(0x76)) {
      Serial.println( "[readBMESensor] BME sensor [0x76] initialisation at complete" );
    } else if (bme280.init(0x77)) {
      Serial.println( "[readBMESensor] BME sensor [0x77] initialisation at complete" );
    } else {
      Serial.println( "[readBMESensor] BME sensor initialisation error" );
    }

    delay(100); // The BME280 needs a moment to get itself together... (50ms is too little time)

    temperature = (int) (10*bme280.getTemperature());
    pressure = (int) (bme280.getPressure() / 91.79F);
    humidity = (int) (bme280.getHumidity());

    outPacket.setPacketType(ATMOSPHERE);
    outPacket.setTemperature(temperature);
    outPacket.setPressure(pressure);
    outPacket.setHumidity(humidity);

    Wire.end();
    digitalWrite(Vext, HIGH);    // Turn off the external power supply

    Serial.println("");
    Serial.print("[readBMESensor] Temperature: ");
    Serial.println((float) temperature/10, 1);
    Serial.print("[readBMESensor] Pressure: ");
    Serial.println(pressure);
    Serial.print("[readBMESensor] Humidity: ");
    Serial.println(humidity);
  }
#endif

void recordPumpStatus() {
  outPacket.setPacketType(PUMP);
  outPacket.setPumpId(pumpId);
  outPacket.setPowerSupply(nodePowerSupply);
  outPacket.setRelayState(relayState);
  outPacket.setControlMode(relayControlMode);
}

void neoPixel(neoPixelColour_t colour) {
  uint8_t red, green, blue;

  switch ( colour ) {
    case NP_OFF: {
      red = 0;
      green = 0;
      blue = 0;
      break;
    }
    case NP_RED: {
      red = 32;
      green = 0;
      blue = 0;
      break;
    }
    case NP_GREEN: {
      red = 0;
      green = 32;
      blue = 0;
      break;
    }
    case NP_BLUE: {
      red = 0;
      green = 0;
      blue = 32;
      break;
    }
    default:
      break;
  }

	neo.begin();                                        // Initialise RGB strip object
	neo.clear();                                        // Set all pixel 'off'
	neo.setPixelColor(0, neo.Color(red, green, blue));  // The first parameter is the pixel index, and we only have one
	neo.show();                                         // Send the updated pixel colors to the hardware.
}

void printSoftRev() {  
  String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);

  Serial.println("[printSoftRev] Print software revision data...");
  Serial.println("[printSoftRev] Sketch   " + sketchRev);
  Serial.println("[printSoftRev] EEPROM   " + eeprom.softwareRevision());
  Serial.println("[printSoftRev] Packet   " + inPacket.softwareRevision(PACKET_HANDLER));
  Serial.println("[printSoftRev] Node     " + inPacket.softwareRevision(NODE_HANDLER));
  Serial.println();
  softRev = false;
}