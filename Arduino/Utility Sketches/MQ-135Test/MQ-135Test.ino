int sensorValue;
int digitalValue;

void setup()
{
  Serial.begin(115200); // sets the serial port to 115200
  pinMode(0, INPUT);
}

void loop()
{
  sensorValue = analogRead(0); // read analog input pin 0
  digitalValue = digitalRead(0);
  Serial.println(sensorValue, DEC); // prints the value read
  Serial.println(digitalValue, DEC);
  delay(10000); // wait 10000ms for next reading
}
