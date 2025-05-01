/*
 nodeV3X1AConfig.h - Langlo Heltec WiFi LoRa 32 V3 Pump controller gateway
 Version 1.0.0	26/03/2023
 
 Created by Peter Harrison, March 2023

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 */

#ifndef nodeV3X1AConfig_h
#define nodeV3X1AConfig_h

// Node MAC & name
const uint32_t myMAC = 0xDC80649C;		// Heltec WiFi LoRa 32 V3 1
const uint32_t pump1MAC = 0xDC822940; // Heltec Wireless Stick Lite V3 Pump Controller 1
const uint32_t pump2MAC = 0xDCA7290F; // Heltec CubeCell Plus Pump Controller 2
const uint32_t pump3MAC = 0xDC032F31; // Heltec CubeCell Pump Controller 3
const uint32_t pump4MAC = 0xDC772E37; // Heltec CubeCell Pump Controller 4
const char* myName = "WiFi LoRa 32 V3 1";

#endif