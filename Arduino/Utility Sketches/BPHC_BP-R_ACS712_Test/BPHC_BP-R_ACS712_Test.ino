// Adapted form https://www.circuitschools.com/measure-ac-current-by-interfacing-acs712-sensor-with-esp32/
// Measuring AC current using ACS712 current sensor with CubeCell V2 Dev-Board

#define pumpControl      GPIO5

int mVperAmp = 100;           // use 185 for 5A IC, 100 for 20A IC and 66 for 30A IC
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
 
void setup() {
  Serial.begin (115200);
  delay(500);
  Serial.println ("ACS712 current sensor");
  uint64_t chipID=getID();
  Serial.printf("ChipID:%04X%08X\r\n",(uint32_t)(chipID>>32),(uint32_t)chipID);

  pinMode( pumpControl,OUTPUT );
  digitalWrite( pumpControl,HIGH );  // Turn the pump ON
}
 
void loop() {
  Serial.println (""); 
  Voltage = getVPP();
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
delay (100);
}
 
// ***** function calls ******
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