/*
This simple sketch can be used to demonstrate an operational RS485 connection.
It uses the CubeCell serial UART, assumes that you have a TTL-to-RS485 adaptor
connected to the Tx/Rx pins on the CubeCell Dev-Board and some other
RS485-connected device that can send and received messages. Set sendLoop to true
to just cycle sending messages or false to send and receive messages manually.
*/

#define sendLoop true
#define RS485BaudRate 9600

int counter = 0;
String dataString;

void setup () {

  // Supply power to the RS485 adaptor

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);

  // Open the serial port at the appropriate baud rate

  Serial.begin(RS485BaudRate);
}

void loop() {
  if ( sendLoop ) {
    // Just loop around sending messages
    Serial.print("[");
    Serial.print(++counter);
    Serial.println("] Hello World!");
    delay(1000);
  } else if ( Serial.available() > 0 ) {
    // If we've received anything, let the sender know...
    dataString = Serial.readString();
    Serial.print("[CubeCell] Message received : ");
    Serial.println(dataString);
  }
}