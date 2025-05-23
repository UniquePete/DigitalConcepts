/*
 * Version for the Pro Mini
 */

#include <Wire.h>
#include <ProMiniPins.h>   // Arduino Pro Mini pin definitions

void setup()
{
  Wire.begin(); // SDA,SCL
 
  Serial.begin(115200);
  delay(500);
  Serial.println("\nI2C Scanner");
  Serial.print("SDA : ");
  Serial.print(SDA);
  Serial.print(", SCL: ");
  Serial.println(SCL);
  Serial.println();
}

 
void loop()
{
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
 
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // wait 5 seconds for next scan
}
