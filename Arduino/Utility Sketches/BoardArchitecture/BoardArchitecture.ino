void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.print("Board architecture: ");
  #if defined(ARDUINO_ARCH_AVR)
    Serial.println("AVR");
    #if defined(__AVR_ATmega328PB__)
      Serial.println("  ATmega328PB");
    #elif defined(__AVR_ATmega328P__)
      Serial.println("  ATmega328P");
      #if defined(PRO) || defined(_PRO_) || defined(__PRO__)
        Serial.println("    Pro Mini");
      #else
        Serial.println("    Other");
      #endif
    #else
      Serial.println("    Other");
    #endif
  #elif defined(ARDUINO_ARCH_SAM)
    Serial.println("SAM");
  #elif defined(ARDUINO_ARCH_STM32)
    Serial.println("STM32");
  #elif defined(ARDUINO_ARCH_ESP32)
    Serial.println("ESP32");
  #elif defined(ARDUINO_ARCH_ESP8266)
    Serial.println("ESP8266");
  #elif defined(ARDUINO_ARCH_ASR)
    Serial.println("ASR");
  #elif defined(ARDUINO_ARCH_ASR_ARDUINO)
    Serial.println("ASR_ARDUINO");
  #elif defined(ARDUINO_ARCH_ASR650x)
    Serial.println("ASR650x");
  #elif defined(ARDUINO_ARCH_ASR6501)
    Serial.println("ASR6502");
  #elif defined(ARDUINO_ARCH_ASR6502)
    Serial.println("ASR6502");
  #else
    Serial.println("Other");
  #endif

  Serial.print("Board: ");
  #if defined(CubeCell_BoardPlus) || defined(CUBECELL_BOARDPLUS)
    Serial.println("CubeCell_BoardPlus");
  #elif defined(WIFI_LoRa_32) || defined(WIFI_LORA_32)
    Serial.println("WIFI_LoRa_32");
  #else
    Serial.println("Other");
  #endif
  
  Serial.print("???: ");
  #if defined(ASR) || defined(_ASR_) || defined(__ASR__)
    Serial.println("ASR");
  #elif defined(__ASR_Arduino__)
    Serial.println("__ASR_Arduino__");
  #else
    Serial.println("Other");
  #endif
  
  Serial.print("Core: ");
  #if defined(asr650x) || defined(ASR650x) || defined(ASR650X) || defined(_ASR650X_) || defined(_ASR650x_)
    Serial.println("ASR650x");
  #else
    Serial.println("Other");
  #endif
  
  Serial.print("MCU: ");
  #if defined(ASR6502)
    Serial.println("ASR6502");
  #else
    Serial.println("Other");
  #endif
  
  Serial.print("Variant: ");
  #if defined(CubeCell_BoardPlus)
    Serial.println("CubeCell_BoardPlus");
  #else
    Serial.println("Other");
  #endif
  
  Serial.print("Architecture: ");
  #if defined(ASR650x_Arduino)
    Serial.println("ASR650x-Arduino");
  #else
    Serial.println("Other");
  #endif
  
}

void loop() {
  // put your main code here, to run repeatedly:

}
