void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[setup] CubeCell Battery Voltage Read");
  uint16_t batteryVoltage = getBatteryVoltage();
  Serial.print("[setup] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
}

void loop() {
}