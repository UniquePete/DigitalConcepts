void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.print("Architecture : ");
#if defined(ARDUINO_ARCH_ESP32)
  Serial.println("ARDUINO_ARCH_ESP32");
  Serial.print("Module : ");
  #if defined(WIFI_LoRa_32_V2)
    Serial.println("WIFI_LoRa_32_V2");
  #elif defined(WIFI_LoRa_32_V3)
    Serial.println("WIFI_LoRa_32_V3 (but could also be Wireless Shell V3 or Wireless Stick Lite V3)");
  #elif
    Serial.println("Not Recognised");
  #endif
#elif
  Serial.println("Not Recognised");
#endif
}

void loop() {
  // put your main code here, to run repeatedly:

}
