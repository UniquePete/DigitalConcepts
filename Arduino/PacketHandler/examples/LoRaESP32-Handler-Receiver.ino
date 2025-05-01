                                                  /* LoRa Receiver Node
 *
 * Function:
 * Receive data forwarded, via LoRa, from remote sensor nodes.
 * 
 * Description:
 * 1. Uses packet structures defined in the PacketHandler library
 * 2. Output generated for Serial Monitor and/or OLED diaplay
 *
 * 7 Oct 2022 v1.0 Code cut down from Gateway sketch
 * 
 * 7 Oct 2022
 * Digital Concepts
 * www.digitalconcepts.net.au
 */

#include <esp_int_wdt.h>        // ESP32 watchdog timers
#include <esp_task_wdt.h>

#include <SSD1306.h>            // OLED display
#include <SPI.h>                // LoRa radio interface
#include <LoRa.h>               // LoRa protocol
#include <PacketHandler.h>      // [Langlo] LoRa Packet management

#include <LangloLoRa.h>         // [Langlo] LoRa configuration parameters
#include <ESP32Pins.h>          // ESP32-WROOM-32 pin definitions
#include <nodeR1config.h>       // Specific local Node configuration parameters

#define wakePin           A4a_A1

bool oledActive = true;

SSD1306 display(0x3c, SDA_OLED, SCL_OLED), *displayPtr;

PacketHandler inPacket;

void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

void setup() {
  
// Get the Serial port working first
  
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("[setup] LoRa [OLED] Receiver");
  Serial.println("[setup] Commence Set-up...");
  
  pinMode(Builtin_LED,OUTPUT);
  pinMode(wakePin,OUTPUT);        // Activate battery monitor
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
  display.drawString(5,5,"LoRa Receiver");
  display.display();

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

  // initialise the Packet object instance
  
  inPacket.begin();

  // and set up the pointers to the OLED display and MQTT client (to pass to the Packet object when required)

  displayPtr = &display;

  // ADC control

  pinMode(A4a_A1,OUTPUT);

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {
 
//  Just loop around checking (promiscuosly) to see if there's anything to process

  if ( receivePacket() ) {
    digitalWrite(Builtin_LED,HIGH);
    Serial.println("[loop] Process the packet...");
         
    // Send relevant information to the Serial Monitor
    inPacket.hexDump();
    inPacket.serialOut();
    // print RSSI & SNR of packet
    Serial.print("[loop] RSSI ");
    Serial.print(LoRa.packetRssi());
    Serial.print("   SNR ");
    Serial.println(LoRa.packetSnr());
    Serial.println();

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

    digitalWrite(Builtin_LED,LOW);
  }
}

bool receivePacket() {
  
// Only if there's something there

  if ( LoRa.parsePacket() ) {
    while ( LoRa.available() ) {
      LoRa.beginPacket();
    
//    Read the packet header
//    We're just reading promiscuously at this point, so we'll actually take anything

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
      
      if ( true ) {

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
        }
       
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
      }
    }
  } else {
    
//  Nothing there

    return false;     
  }
}
