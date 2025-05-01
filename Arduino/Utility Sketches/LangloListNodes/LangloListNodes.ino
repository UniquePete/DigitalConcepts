#include "LangloNodeList.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:
  for (int i = 0; i < nodeCount; i++) {
    Serial.print("Node ");
    Serial.print(i);
    Serial.print("  MAC: ");
    Serial.print(nodeList[i].MAC, HEX);
    Serial.print(", Timer: ");
    Serial.print(nodeList[i].timer);
    Serial.print(", Topic: ");
    Serial.println(nodeList[i].topic);
  }

  Serial.println();

  uint32_t nodeMacAddress = 0xDC2EE2F7;
  int nodeArrayIndex = indexOf(nodeMacAddress);

  Serial.println("Searching for MAC 0xDC2EE2F7... ");
  Serial.print("Found at index ");
  Serial.print(nodeArrayIndex);
  Serial.print(", Topic is: ");
  Serial.println(nodeList[nodeArrayIndex].topic);
  
  while (true) {
    
  }
}
