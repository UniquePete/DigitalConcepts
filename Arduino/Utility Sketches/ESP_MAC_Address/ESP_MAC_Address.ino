// Complete Instructions to Get and Change ESP MAC Address: https://RandomNerdTutorials.com/get-change-esp32-esp8266-mac-address-arduino/

#ifdef ESP32
  #include <WiFi.h>
  uint32_t chipId = 0;
#else
  #include <ESP8266WiFi.h>
#endif

void setup(){
  Serial.begin(115200);
  Serial.println();
  #ifdef ESP32
    for(int i=0; i<17; i=i+8) {
      chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
  
    Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("This chip has %d cores\n", ESP.getChipCores());
    Serial.print("Chip ID: "); Serial.println(chipId);
    Serial.print("ESP32 MAC Address:  ");
  #else
    Serial.print("ESP8266 MAC Address:  ");
  #endif

  Serial.println(WiFi.macAddress());
}
 
void loop(){

}
