#include <softSerial.h>

/*
   A simple sketch to test RS485 communications with Modbus RTU sensors.
   It uses SoftwareSerial, and assumes that we have a TTL-to-RS485 adaptor connected
   to the second CubeCell Dev-Board Plus UART (UART_TX2/UART_RX2) and the sensor under
   test.
*/

// Define baud rate as appropriate
// 7-in-1 Soil Condition          4800 (default)
// SHT30 Temperature & Humidity   9600 (default)
// QDY30A Water Level             9600 (default)

#define RS485BaudRate 9600

int counter = 0;
String receiveString, sendString;

// The following are valid query strings for the listed sensors. Remove the comment prefix
// from the query that is to be sent. For more detail, refer to the pages for individual
// sensors.

// 7-in-1 Soil Condition
//byte queryData[]{ 0xFF, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x91, 0x59 }; // Broadcast Enquire Slave ID
//byte queryData[]{ 0x01, 0x06, 0x07, 0xD0, 0x00, 0x02, 0x08, 0x86 }; // Set Slave ID to 2
//byte queryData[]{ 0x01, 0x06, 0x07, 0xD1, 0x00, 0x02, 0x59, 0x46 }; // Set Baud to 9600

// SHT30 Temperature & Humidity
//byte queryData[]{ 0x00, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x85, 0x56 }; // Broadcast Enquire Slave ID

// QDY30A Water Level
byte queryData[]{ 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xDB }; // Broadcast Enquire Slave ID

byte receivedData[20];  // Size set as required by individual sensors

// Define the serial entity for connection to the RS485 adaptor

softSerial RS485(UART_TX2, UART_RX2);

void setup() {

  // Supply power to the RS485 adaptor

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
 
  // Open the serial ports

  Serial.begin(115200);        // To the Serial Monitor
  RS485.begin(RS485BaudRate);  // To the RS485 bus
  delay(500);

  Serial.println("[setup] CubeCell Plus RS485 Sensor Test");
}

void loop() {

  // Send the query...
  // Note that a Set query (e.g. baud rate or Slave ID) may result in loss of
  // communication until sketch RS485BaudRate or query strings are adjusted to match
  
  Serial.println("[loop] Sending query...");
  RS485.write(queryData, sizeof(queryData));
  
  delay(500);  // Will need to wait a little before checking for a response...
  
  // Check if we've received anything...
  
  if ( RS485.available() > 0 ) {
  
    // Report anything we receive on the RS485 bus
    
    RS485.readBytes(receivedData, sizeof(receivedData));
    Serial.print("[loop] Received Data : ");
    for ( int i = 0; i <= sizeof(receivedData); i++ ) {
      Serial.print("0x");
      Serial.print(receivedData[i],HEX);
      if ( i < sizeof(receivedData) ) {
        Serial.print(", ");
      }
    }
    Serial.println();

  } else {
    Serial.println("[loop] No response...");
  }
  delay(5000);
}