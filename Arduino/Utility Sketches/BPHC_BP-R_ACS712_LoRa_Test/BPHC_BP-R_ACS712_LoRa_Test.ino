// Adapted form https://www.circuitschools.com/measure-ac-current-by-interfacing-acs712-sensor-with-esp32/
// Measuring AC current using ACS712 current sensor with CubeCell V2 Dev-Board
// Add capability to transmit data via LoRa (when operating on battery alone - i.e. after power failure)
#include <LoRa_APP.h>
#include <LangloLoRa.h>         // LoRa configuration parameters
#include <PacketHandler.h>      // [Langlo] LoRa Packet management
#include <CubeCellV2Pins.h>     // Heltec CubeCell V2 pin definitions

#define pumpControl      GPIO5

uint32_t gatewayMAC = 0xDC80649C;
uint32_t myMAC = 0xDC772E37;
uint16_t messageCounter = 0;

PacketHandler outPacket;

int mVperAmp = 100;           // use 185 for 5A IC, 100 for 20A IC and 66 for 30A IC
int Watt = 0;
double Voltage = 0;
uint16_t reportedVoltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

uint8_t pumpId = 4;
PH_powerState pumpPowerState = PH_POWER_FAIL;
PH_controlMode pumpControlMode = PH_MODE_REMOTE;

static RadioEvents_t RadioEvents;
bool lora_idle = true;
void OnTxDone( void );
void OnTxTimeout( void );
int16_t rssi,rxSize;
 
void setup() {
  boardInitMcu();
  Serial.begin (115200);
  delay(500);
  Serial.println ("ACS712 current sensor");
  uint64_t chipID=getID();
  Serial.printf("ChipID:%04X%08X\r\n",(uint32_t)(chipID>>32),(uint32_t)chipID);

  pinMode( pumpControl,OUTPUT );
  digitalWrite( pumpControl,HIGH );  // Turn the pump ON

//  Initialise LoRa

  Serial.println("[setup] Initialise LoRa...");

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  Radio.Init( &RadioEvents );
  
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                  LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                  LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                  0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  Radio.Sleep( );
  rssi=0;

  Serial.println("[setup] LoRa initialisation complete");
  Serial.println();

// Initialise the Packet object instances
  
  outPacket.begin(gatewayMAC,myMAC,messageCounter);

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}
 
void loop() {
  Serial.println (""); 
  Voltage = getVPP();
  reportedVoltage = 1000 * Voltage;

  Serial.println("[loop] Begin LoRa transmission...");
  outPacket.setSequenceNumber(messageCounter);
  outPacket.setPacketType(PUMP);
  outPacket.setPumpId(pumpId);

  if ( reportedVoltage == 0 ) {
    Serial.println("[loop] Power Failure!");
    pumpPowerState = PH_POWER_OFF;
    pumpPowerState = PH_POWER_FAIL;
  } else {
    Serial.println("[loop] Power present...");
    pumpPowerState = PH_POWER_ON;
    pumpControlMode = PH_MODE_REMOTE;
  }

  outPacket.setPowerState(pumpPowerState);
  outPacket.setControlMode(pumpControlMode);
  outPacket.hexDump();
  outPacket.serialOut();
  sendMessage(outPacket);
  messageCounter++;                 // Increment message count

  VRMS = (Voltage/2.0) * 0.707;   //root 2 is 0.707
  AmpsRMS = ((VRMS * 1000)/mVperAmp) - 0.06; //0.3 is the error I got for my sensor
 
  Serial.print(AmpsRMS);
  Serial.print(" Amps RMS  ---  ");
  Watt = (AmpsRMS*240/1.2);
  // note: 1.2 is my own empirically established calibration factor
  // as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
  // 240 is the main AC power voltage – this parameter changes locally
  Serial.print(Watt);
  Serial.println(" Watts");
  delay (10000);
}

void OnTxDone( void ) {
  Serial.println("[OnTxDone] TX done!");
}

void OnTxTimeout( void ) {
  Serial.println("[OnTxTimeout] TX Timeout...");
}

float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here (will be reset to the max reading taken)
  int minValue = 4096;          // store min value here - CubeCell ADC resolution - (will be reset to the min reading taken)
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(ADC);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.3)/4096.0; //ASR650x ADC resolution 4096
      
   return result;
 }

void sendMessage(PacketHandler packet) {
  turnOnRGB(COLOR_SEND,0);
  Radio.Send(packet.byteStream(), packet.packetByteCount());
  turnOffRGB();
  Serial.println("[sendMessage] Packet Sent");
  Serial.println();
}