#include <softSerial.h>

/*
   A simple sketch to test RS485 communications with SHT30 temperature & humidity sensor.
   It uses SoftwareSerial, and assumes that we have a TTL-to-RS485 adaptor connected
   to the second CubeCell Dev-Board Plus UART (UART_TX2/UART_RX2) and the SHT30 sensor.
*/

#define RS485BaudRate 9600

int counter = 0;
String receiveString, sendString;
// 7-in-1 Soil Condition
//byte queryData[]{ 0x01, 0x03, 0x00, 0x00, 0x00, 0x04, 0x43, 0x09 }; // Master read all
//byte queryData[]{ 0xFF, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x91, 0x59 }; // Enquire Slave ID (Reports Slave ID — Default 1)
//byte queryData[]{ 0x00, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x85, 0x56 }; // Enquire Slave ID — No repsonse!
//byte queryData[]{ 0x01, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x84, 0x87 }; // Enquire Slave ID
//byte queryData[]{ 0x01, 0x06, 0x07, 0xD0, 0x00, 0x02, 0x08, 0x86 }; // Set Slave ID to 2 (works!)
//byte queryData[]{ 0x01, 0x03, 0x07, 0xD1, 0x00, 0x01, 0xD5, 0x47 }; // Enquire Baud
//byte queryData[]{ 0x01, 0x06, 0x07, 0xD1, 0x00, 0x02, 0x59, 0x46 }; // Set Baud to 9600 (works!)

// SHT30 Temperature & Humidity
//byte queryData[]{ 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B }; // Master read SHT30 temperature humidity
//byte queryData[]{ 0x01, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x84, 0x87 }; // Enquire Slave ID
//byte queryData[]{ 0x00, 0x03, 0x07, 0xD0, 0x00, 0x01, 0x85, 0x56 }; // Enquire Slave ID (Reports Slave ID 0)
//byte queryData[]{ 0x01, 0x06, 0x07, 0xD0, 0x00, 0x02, 0x08, 0x86 }; // Set Slave ID to 2 (doesn't seem to have any effect)
//byte queryData[]{ 0x01, 0x03, 0x07, 0xD1, 0x00, 0x01, 0xD5, 0x47 }; // Enquire Baud

// Water Level
//byte queryData[]{ 0x01, 0x03, 0x00, 0x04, 0x00, 0x01, 0xC5, 0xCB }; // Water Level
//byte queryData[]{ 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xDB }; // Enquire Slave ID
byte queryData[]{ 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A }; // Enquire Slave ID
//byte queryData[]{ 0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0x85, 0xE8 }; // Enquire Slave ID
//byte queryData[]{ 0x01, 0x06, 0x00, 0x00, 0x00, 0x03, 0x08, 0x0B }; // Set Slave ID to 3 (doesn't seem to have any effect)

byte receivedData[19];
// Define the serial entity for connection to the RS485 adaptor
softSerial RS485(UART_TX2, UART_RX2);

void setup() {

  // Supply power to the RS485 adaptor

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
 
  // Open the serial ports

  Serial.begin(115200);
  RS485.begin(RS485BaudRate);

  Serial.println("[setup] CubeCell Plus RS485 Sensor Test");
}

void loop() {
  // Send the query...
  Serial.println("[loop] Sending query...");
  RS485.write(queryData, sizeof(queryData));
  delay(500);
  // If we've received anything...
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
    Serial.println("\n");
        // Parse and print the received data in decimal format
    unsigned int soilHumidity = (receivedData[3] << 8) | receivedData[4];
    unsigned int soilTemperature = (receivedData[5] << 8) | receivedData[6];
    /*
    unsigned int soilConductivity = (receivedData[7] << 8) | receivedData[8];
    unsigned int soilPH = (receivedData[9] << 8) | receivedData[10];
    unsigned int nitrogen = (receivedData[11] << 8) | receivedData[12];
    unsigned int phosphorus = (receivedData[13] << 8) | receivedData[14];
    unsigned int potassium = (receivedData[15] << 8) | receivedData[16];
    */
    Serial.print("Soil Humidity: ");
    Serial.print((float)soilHumidity / 10.0);
    Serial.println("%");
    Serial.print("Soil Temperature: ");
    Serial.print((float)soilTemperature / 10.0);
    Serial.println("°C");
    /*
    Serial.print("Soil Conductivity: ");
    Serial.println(soilConductivity);
    Serial.print("Soil pH: ");
    Serial.println((float)soilPH / 10.0);
    Serial.print("Nitrogen: ");
    Serial.println(nitrogen);
    Serial.print("Phosphorus: ");
    Serial.println(phosphorus);
    Serial.print("Potassium: ");
    Serial.println(potassium);
    */
    Serial.println();

  } else {
    Serial.println("[loop] No response...");
  }
  delay(5000);
}