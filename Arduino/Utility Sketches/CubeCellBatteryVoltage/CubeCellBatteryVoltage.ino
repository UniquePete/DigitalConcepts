/* CubeCell Battery Voltage
 *
 * Function:
 * Read battery voltage via internal circuit
 * 
 * Digital Concepts
 * digitalconcepts.net.au
 *
 */

uint16_t batteryVoltage = 0;

void setup() {
  Serial.begin(115200);
  delay(200);                 // Let things settle down before we try to send anything out...
  Serial.println();
  Serial.println("[setup] CubeCell Battery Voltage Read");
  Serial.print("[setup] Vext Pin : ");
  Serial.println(Vext);
  Serial.print("[setup] VBAT_ADC_CTL Pin : ");
  Serial.println(VBAT_ADC_CTL);
  Serial.print("[setup] P3_3 : ");
  Serial.println(P3_3);
  Serial.print("[setup] ADC : ");
  Serial.println(ADC);
  Serial.print("[setup] P2_3 : ");
  Serial.println(P2_3);
//  Serial.print("[setup] ADC_VBAT : ");
//  Serial.println(ADC_VBAT);

  pinMode(VBAT_ADC_CTL,OUTPUT);
  Serial.println("[setup] Set VBAT_ADC_CTL LOW");
  digitalWrite(VBAT_ADC_CTL,LOW);
  batteryVoltage=getBatteryVoltage();
  Serial.print("[setup] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
  
  Serial.println("[setup] Set VBAT_ADC_CTL HIGH");
  digitalWrite(VBAT_ADC_CTL,HIGH);
  batteryVoltage=getBatteryVoltage();
  Serial.print("[setup] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
 }

void loop()
{
  batteryVoltage=getBatteryVoltage();
  Serial.print("[loop] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
  delay(5000);
}