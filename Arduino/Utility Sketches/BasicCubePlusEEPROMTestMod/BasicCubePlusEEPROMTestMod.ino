/* 
 *  Test Write & Read from external [I2C) EEPROM
 */

// Incomplete!!

#include <Wire.h>
#include <AT24C32N.h>           // EEPROM
//#include "CubeCellPlusPins.h" // CubeCell Plus pin definitions
#include "CubeCellPins.h" // CubeCell pin definitions

EEPROM_AT24C32N bhcpEEPROM;
#define I2C_EEPROM_Address  0x50

#define sequenceLocation     0
#define sequenceBytes 2

union EEPROMPayload {
  char payloadByte[2];
  uint16_t counter;
};

EEPROMPayload EEPROMWriteData, EEPROMReadData;

uint16_t messageCounter = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  
  Serial.print("[setup] ");
//  Serial.print(myName);
  Serial.println(" CubeCell [Plus] I2C EEPROM Test");
  Serial.println();
  Serial.println("[setup] Set-up Complete");
  Serial.println(); 
}

void loop() {
//  Serial.println("[loop] Enter Loop");
      Serial.print("[loop] Read Sequence Number       ");
      digitalWrite(Vext, LOW);    // Turn on the external power supply
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();               // On with the show...
      bhcpEEPROM.RetrieveBytes(EEPROMReadData.payloadByte, sequenceBytes, sequenceLocation, false);
      Wire.end();
      Serial.println(EEPROMReadData.counter);
      messageCounter = EEPROMReadData.counter + 1;

      batteryVoltage=getBatteryVoltage();
      Serial.print("[loop-TX] Battery Voltage: ");
      Serial.print( batteryVoltage );
      Serial.println(" mV");

      Serial.println("[loop-TX] Send Voltage Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage(voltageTypePacket, messageCounter);
      turnOffRGB();
/*      
      // Have a snooze while the gateway hadndles that one
      delay(100);
      
      readDS18B20Sensors();
        
      Serial.println("[loop-TX] Send Temperature Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage(temperatureTypePacket, messageCounter);
      turnOffRGB();
      
      // Have a snooze while the gateway hadndles that one
      delay(100);
*/

      readDS18B20Sensors();
        
      Serial.println("[loop-TX] Send Temperature Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage(temperatureTypePacket, messageCounter);
      turnOffRGB();

      awtsStatus();
        
      Serial.println("[loop-TX] Send AWTS Message");
      turnOnRGB(COLOR_SEND,0);
      sendMessage(awtsTypePacket, messageCounter);
      turnOffRGB();

      digitalWrite(Vext, LOW);    // Make sure the external power supply is on
      delay(10);                  // Give everything a moment to settle down
      Wire.begin();               // On with the show...
      EEPROMWriteData.counter = messageCounter;
      bhcpEEPROM.WriteBytes(sequenceLocation, sequenceBytes, EEPROMWriteData.payloadByte);
      Wire.end();                 // All done here

      Serial.println(""); 
      state=LOWPOWER;
      
      TimerStart( &sleep ); // Doing this any sooner seems to stuff up the Dallas Temperature library
      
	    break;
    }
		case LOWPOWER: {
      lowPowerHandler();
      break;
    }
    default:
        break;
	}
//    Radio.IrqProcess( );
}

void onTxDone(void)
{
  Serial.println("[OnTxDone] TX done!");
  Radio.Sleep( );
}

void onTxTimeout(void)
{
  Serial.println("[OnTxTimeout] TX Timeout...");
  Radio.Sleep( );
}

void readDS18B20Sensors() {
  Serial.println("[readDS18B20Sensors] Read sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/
  digitalWrite(Vext, LOW);    // Make sure the external power supply is on
  delay(10);
  ds18b20Sensors.begin();
  delay(100);                 // Give everything a moment to settle down
  
  ds18b20Sensors.requestTemperatures(); // Send the command to get temperatures
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.println("[readDS18B20Sensors] Device 1 (index 0)");

  float tempTemperature = ds18b20Sensors.getTempCByIndex(0);
  Serial.print("[readDS18B20Sensors] Returned value: ");
  Serial.println(tempTemperature);

  nodeTemperature = (int) (10*(tempTemperature + 0.05));
 
  Serial.print("[readDS18B20Sensors] Temperature: ");
  Serial.println((float) nodeTemperature/10, 1);
}

void awtsStatus () {
  const int sampleInterval = 500;  // Increase value to slow down activity
  const int loopCycleLimit = 4;    // # samples before recording value
  bool keepGoing = true;
  int loopCycles = 0;
  unsigned long t = 0;
  float sensorValue = 0.0;

  long duration = 0;                   // Sound wave travel time
  int distance = 0;                    // Distance measurement

  digitalWrite(Vext, LOW);         // Make sure the external power supply is on
  delay(100);                      // Give everything a moment to settle down
  
  LoadCell.begin();
  LoadCell.start(stabilisingTime, _tare);
  LoadCell.setCalFactor(calibrationValue);
//  Serial.println("[awtsStatus] Startup is complete");

  // Clear the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  while (keepGoing) {
    // Check for new data/start next conversion:
    if (LoadCell.update()) {
      newDataReady = true;
    }
    // Get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + sampleInterval) {
        sensorValue = LoadCell.getData();
        Serial.print("[awtsStatus] Pressure reading: ");
        Serial.println(sensorValue);
        newDataReady = false;
        t = millis();
          
        // Take an IT depth measurement while we're waiting
        // Set the trigPin HIGH (ACTIVE) for 10 microseconds
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        // Read the echoPin, returns the sound wave travel time in microseconds
        duration += pulseIn(echoPin, HIGH);
        Serial.print("[awtsStatus] Distance: ");
        Serial.println((duration/(loopCycles+1)) * 0.034 / 2);

        if (++loopCycles > loopCycleLimit) {  // Take several samples before returning a reading
          keepGoing = false;
          blowerPressure = (int16)(100 * sensorValue);
          Serial.print("[awtsStatus] Total readings: ");
          Serial.println(loopCycles);
          duration = (long)(duration / loopCycles);
        }
      }
    }
  }
  newDataReady = false;
 
  // Calculate the IT level
  distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (out and back)
  Serial.print("[awtsStatus] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  if (distance > (itHeight + itLevelOffset)) distance = itHeight + itLevelOffset;  // This shouldn't happen, but...
  itLevel = (int) (100 * (itHeight - (distance-itLevelOffset)) / itHeight) + 0.5;
  Serial.print("[awtsStatus] IT Height: ");
  Serial.print(itHeight);
  Serial.println(" cm");
  Serial.print("[awtsStatus] IT Level: ");
  Serial.print(itLevel);
  Serial.println(" %");
}

void blowerStatus () {
  const int sampleInterval = 500;  // Increase value to slow down activity
  const int loopCycleLimit = 4;    // # samples before recording value
  bool keepGoing = true;
  int loopCycles = 0;
  unsigned long t = 0;
  float sensorValue = 0.0;
  
  digitalWrite(Vext, LOW);        // Make sure the external power supply is on
  delay(100);                     // Give everything a moment to settle down
  
  LoadCell.begin();
  LoadCell.start(stabilisingTime, _tare);
  LoadCell.setCalFactor(calibrationValue);
//  Serial.println("[awtsStatus] Startup is complete");

  while (keepGoing) {
    // Check for new data/start next conversion:
    if (LoadCell.update()) {
      newDataReady = true;
    }
    // Get smoothed value from the dataset:
    if (newDataReady) {
      if (millis() > t + sampleInterval) {
        sensorValue = LoadCell.getData();
        Serial.print("[blowerStatus] Load_cell output val: ");
        Serial.println(sensorValue);
        newDataReady = false;
        t = millis();        
        if (++loopCycles > loopCycleLimit) {  // Take several samples before returning a reading
          keepGoing = false;
          blowerPressure = (int16)(100 * sensorValue);
        }
      }
    }
  }
  newDataReady = false;
 }

void itStatus () {
  long duration = 0;                   // Sound wave travel time
  int distance = 0;                    // Distance measurement

  // Clear the trigPin condition
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigPin HIGH (ACTIVE) for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distance = duration * 0.034 / 2;  // Speed of sound wave divided by 2 (out and back)
  Serial.print("[itStatus] Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  itLevel = (int) 100 * (itHeight - (distance-itLevelOffset)) / itHeight;
  Serial.print("[itStatus] IT Height: ");
  Serial.print(itHeight);
  Serial.println(" cm");
  Serial.print("[itStatus] IT Level: ");
  Serial.print(itLevel);
  Serial.println(" %");
}

void sendMessage(uint8_t packetType, int counter) {

  outgoingPacket.content.destinationMAC = gatewayMAC;
  outgoingPacket.content.sourceMAC = myMAC;
  outgoingPacket.content.sequenceNumber = (uint16_t) counter % 65536;
  outgoingPacket.content.type = packetType;
  outgoingPacket.content.byteCount = (uint8_t) bytesIn(packetType);

  switch (packetType) {
    case powerTypePacket:
// Not required
      break;
    case voltageTypePacket:
    {
      voltagePayload payload;
      payload.source.voltage = batteryVoltage;
      Serial.print("[sendMessage] Sending ");
      Serial.print(myName);
      Serial.print(" voltage message - Sequence Number ");
      Serial.println(counter % 65536);
      for (int i = 0; i < outgoingPacket.content.byteCount; i++) {
        outgoingPacket.content.payload[i] = payload.payloadByte[i];
      }
      break;
    }
    case tankTypePacket:
// Not required
      break;
    case pumpTypePacket:
// Not required
      break;
    case weatherTypePacket:
// Not required
      break;
    case atmosphereTypePacket:
 // Not required
      break;
    case temperatureTypePacket:
      temperaturePayload payload;
      payload.recorded.temperature = nodeTemperature;
      Serial.print("[sendMessage] Sending ");
      Serial.print(myName);
      Serial.print(" temperature sensor message - Sequence Number ");
      Serial.println(counter % 65536);
      for (int i = 0; i < outgoingPacket.content.byteCount; i++) {
        outgoingPacket.content.payload[i] = payload.payloadByte[i];
      }
      break;
    case rainfallTypePacket:
// Not required
      break;
    case windTypePacket:
// Not required
      break;
    case sprinklerTypePacket:
// Not required
      break;
    case awtsTypePacket:
    {
      awtsPayload payload;
      payload.recorded.blowerPressure = blowerPressure;
      payload.recorded.itLevel = itLevel;
      Serial.print("[sendMessage] Sending ");
      Serial.print(myName);
      Serial.print(" sensor message - Sequence Number ");
      Serial.println(counter % 65536);
      for (int i = 0; i < outgoingPacket.content.byteCount; i++) {
        outgoingPacket.content.payload[i] = payload.payloadByte[i];
      }
      break;
    }
    default:
      Serial.println("[sendMessage] No payload...");
  }

// Finally, calculate the checksum

  outgoingPacket.content.checksum = generateChecksum(outgoingPacket);

  Serial.println("[sendMessage] Outgoing packet: ");
//  digitalWrite(DevKit_Builtin_LED, LOW);  // Turn on the LED to signal sending
/*  
// Check that no one else is transmitting first
  while ((bool) LoRa.available()) {
    Serial.println("[sendMessage] Busy...");
// It would take 212 ms to transmit a [max size] 255 byte packet at 9600 bps
    delay(random(200));
  }
*/  
  int totalByteCount = LGO_HeaderSize + outgoingPacket.content.byteCount;
  Serial.print("[sendMessage] Byte Count: ");
  Serial.println( totalByteCount );
  for (int i = 0; i < totalByteCount; i++) {
    Serial.printf( "%02X", outgoingPacket.packetByte[i]);
    if ((i % 4) == 3) {
      Serial.println("    ");
    } else {
      Serial.print("  ");
    }
  }
  if ((totalByteCount % 4) != 0) Serial.println();

  Serial.println("[sendMessage] Finished Packet");
  Radio.Send( (uint8_t *)outgoingPacket.packetByte, totalByteCount );
  Serial.println("[sendMessage] Packet Sent");
  Serial.println();
}

uint32_t generateChecksum(dataPacket& outgoingPacket) {
  
//  Generate the checksum on the outgoing packet

  CRC32 crc;
  int checksumStart = LGO_HeaderSize - 4;
  int checksumEnd = outgoingPacket.content.byteCount + 4;
  for (int i = checksumStart; i < checksumEnd; i++) {
    crc.update(outgoingPacket.packetByte[i]);
  }
  uint32_t packetChecksum = crc.finalize();
  return packetChecksum;
}

int bytesIn(uint8_t packetType) {
  int result;
  switch (packetType) {
    case powerTypePacket:
      result = powerPayloadBytes;
      break;
    case voltageTypePacket:
      result = voltagePayloadBytes;
      break;
    case tankTypePacket:
      result = tankPayloadBytes;
      break;
    case pumpTypePacket:
      result = pumpPayloadBytes;
      break;
    case weatherTypePacket:
      result = weatherPayloadBytes;
      break;
    case temperatureTypePacket:
      result = temperaturePayloadBytes;
      break;
    case atmosphereTypePacket:
      result = atmospherePayloadBytes;
      break;
    case rainfallTypePacket:
      result = rainfallPayloadBytes;
      break;
    case windTypePacket:
      result = windPayloadBytes;
      break;
    case sprinklerTypePacket:
      result = sprinklerPayloadBytes;
      break;
    case awtsTypePacket:
      result = awtsPayloadBytes;
      break;
    default:
      result = defaultPayloadBytes;
  }
  return result;
}
