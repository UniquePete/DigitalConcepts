/*
  Basic I2C Port Scanner for CubeCell

  Function:
  Scan nominated I2C bus for active devices

  2 Dec 2023
  Digital Concepts
  www.digitalconcepts.net.au
*/

#include <Wire.h>
//#include <CubeCellPins.h>   			// CubeCell pin definitions
#include <CubeCellPlusPins.h>   // CubeCell Plus pin definitions

void setup() {
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,LOW); 
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] CubeCell I2C Scanner");
  Serial.println("[setup] Power up the I2C bus...");
  Serial.print("        SDA : ");
  Serial.print(SDA);
  Serial.print(", SCL: ");
  Serial.println(SCL);
  Wire.begin(); // SDA,SCL
}
  
void loop() {
  byte error, address;
  int nDevices;
 
  Serial.println("[loop] Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
 
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)  {
      Serial.print("[loop] I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    } else if (error==4) {
      Serial.print("[loop] Unknown error [4] at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
//    } else {
//      Serial.print("[loop] Error ");
//      Serial.print(error);
//      Serial.print(" at address 0x");
//      if (address<16) 
//        Serial.print("0");
//      Serial.println(address,HEX);
    }  
  }
  if (nDevices == 0)
    Serial.println("[loop] No I2C devices found\n");
  else
    Serial.println("[loop] Done");
 
  delay(5000);           // wait 5 seconds for next scan
}
