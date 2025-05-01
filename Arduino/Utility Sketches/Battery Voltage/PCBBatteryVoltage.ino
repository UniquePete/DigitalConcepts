#define batteryMonitor  36
#define wakeControl     13

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("[setup] Battery Voltage Read");
  uint16_t batteryVoltage = readBatteryVoltage();
  Serial.print("[setup] Battery Voltage: ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
}

void loop() {
}

uint16_t readBatteryVoltage() {
  // ADC resolution
  const int resolution = 12;
  const int adcMax = pow(2,resolution) - 1;
  const float adcMaxVoltage = 3.3;
  // On-board voltage divider
  const int R1 = 27;
  const int R2 = 100;
  // Calibration measurements
  const float measuredVoltage = 4.2;
  const float reportedVoltage = 4.14;
  // Measured value multiplication factor
  const float factor = (adcMaxVoltage / adcMax) * ((R1 + R2)/(float)R2) * (measuredVoltage / reportedVoltage);

  float averageReading;
  uint16_t analogValue = 0;
  uint16_t voltage = 0;

  digitalWrite(wakeControl,HIGH);
  delay(500); // Wait a moment for things to settle down

  // The ESP32 doesn't have a particularly good ADC, so we calculate an average

  int averageCount = 10;
  for (int i = 0; i < averageCount; i++) {
    analogValue += analogRead(batteryMonitor); 
    delay(10);
  }
  averageReading = analogValue / (float)averageCount;

  digitalWrite(wakeControl,LOW); 
  voltage = (unit16_t)(averageReading * factor * 1000); // milliVolts
  return voltage;
}