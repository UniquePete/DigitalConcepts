#include <Arduino.h>
#include <Wire.h>        // I2C bus
#include <SSD1306.h>     // OLED display

#define Vbat_Read   37
#define	ADC_Ctrl    21

SSD1306 myDisplay(0x3c, SDA_OLED, SCL_OLED);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.print("[setup] Battery Voltage Read");
  Serial.println();
  Serial.println("[setup] Commence Set-up...");

  pinMode(RST_OLED,OUTPUT);     // GPIO16
  digitalWrite(RST_OLED,LOW);   // Set GPIO16 LOW to reset OLED
  delay(50);
  digitalWrite(RST_OLED,HIGH);

  myDisplay.init();
  myDisplay.clear();
  myDisplay.setFont(ArialMT_Plain_16);
  myDisplay.setTextAlignment(TEXT_ALIGN_LEFT);

  pinMode(ADC_Ctrl,OUTPUT);
  pinMode(VBAT_Read,INPUT);
  adcAttachPin(VBAT_Read);
  analogReadResolution(12);
  readBatteryVoltage();

  Serial.println("[setup] Set-up Complete");
  Serial.println();
}

void loop() {
}

uint16_t readBatteryVoltage() {
  // ADC resolution
  const int resolution = 12;
  const int adcMax = pow(2,resolution) - 1;
  const float adcMaxVoltage = 3.3;
  // On-board voltage divider
  const int R1 = 220;
  const int R2 = 100;
  // Calibration measurements
  const float measuredVoltage = 4.2;
  const float reportedVoltage = 3.82;
  // Calibration factor
  const float factor = (adcMaxVoltage / adcMax) * ((R1 + R2)/(float)R2) * (measuredVoltage / reportedVoltage); 
  digitalWrite(ADC_Ctrl,LOW);
  delay(100);
  int analogValue = analogRead(VBAT_Read);
  digitalWrite(ADC_Ctrl,HIGH);

  float floatVoltage = factor * analogValue;
  uint16_t voltage = (int)(floatVoltage * 1000.0);

  Serial.print("[readBatteryVoltage] ADC : ");
  Serial.println(analogValue);
  Serial.print("[readBatteryVoltage] Float : ");
  Serial.println(floatVoltage,3);
  Serial.print("[readBatteryVoltage] milliVolts : ");
  Serial.println(voltage);

  myDisplay.clear();
  myDisplay.drawString(0,5,"Battery Level");
  myDisplay.display();
  myDisplay.drawString(0,25, String(floatVoltage) + " Volts");
  myDisplay.display();

  return voltage;
}	