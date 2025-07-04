#include <softSerial.h>

/*
   This simple sketch can be used to demonstrate an operational RS485 connection.
   It uses SoftwareSerial, and assumes that you have a TTL-to-RS485 adaptor connected
   to the second CubeCell Dev-Board Plus UART (UART_TX2/UART_RX2) and some other
   RS485-connected device that can send and received messages. Set sendLoop to true
   to just cycle sending messages or false to send and receive messages manually.
*/

#define sendLoop true
#define RS485BaudRate 9600

int counter = 0;
String receiveString, sendString;

// Define the serial entity for connection to the RS485 adaptor
softSerial RS485(UART_TX2, UART_RX2);

void setup() {

  // Supply power to the RS485 adaptor

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
 
  // Open the serial ports

  Serial.begin(115200);
  RS485.begin(RS485BaudRate);

  Serial.println("[setup] CubeCell Plus RS485 Test");
}

void loop() {
  if ( sendLoop ) {
    // Just loop around sending messages
    writeHelloWorld();
  } else {
    // If we've got something to send...
    if ( Serial.available() > 0 ) {
    // Read input from the local Serial Monitor
      sendString = Serial.readString();
    // echo the input locally
      Serial.print("[loop] Input string: ");
      Serial.println(sendString);
    // then send it out on the RS485 bus
      processInput(sendString);
      promptForInput();
    }
    // If we've received anything...
    if ( RS485.available() > 0 ) {
      // Report anything we receive on the RS485 bus
      receiveString = RS485.readString();
      Serial.print("[loop] Received string: ");
      Serial.println(receiveString);
    }
  }
  delay(1000);
}

void writeHelloWorld() {
  Serial.print("[loop] Sending '[");
  Serial.print(++counter);
  Serial.println("] Hello World!'...");
  RS485.print("[");
  RS485.print(counter);
  RS485.println("] Hello World!");
}

void processInput( String inputString ) {
  RS485.print("[From Remote Host] ");
  RS485.println(inputString);
}

void promptForInput() {
    Serial.println("[loop] Type in your message and press <return>...");
}