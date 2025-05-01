// Langlo LoRa configuration parameters

#ifndef LangloLoRa_h
#define LangloLoRa_h

// ESP32-LoRa & RFM95W parameters

#define BAND					917E6
// #define SignalBandwidth			125E3
// #define SpreadingFactor			7
// #define CodingRateDenominator	5
// #define PreambleLength			8
// #define SyncWord				0x12

bool PABOOST = true;

// ESP32-LoRa setup code (include in sketch)

//  SPI.begin(5,19,27,18);
//  LoRa.setPins(CS,RST,DI0);
//  if (!LoRa.begin(BAND)) {
//    Serial.println("[setup] Starting LoRa failed!");
//    while (true);
//  }
//  LoRa.setSpreadingFactor(SpreadingFactor);
//  LoRa.setSignalBandwidth(SignalBandwidth);
//  LoRa.setPreambleLength(PreambleLength);
//  LoRa.setCodingRate4(CodingRateDenominator);
//  LoRa.setSyncWord(SyncWord);
//  LoRa.disableCrc();

// CubeCell parameters

#define RF_FREQUENCY                  917000000	// Hz
#define LORA_BANDWIDTH                0         // [0: 125 kHz,
                                                //  1: 250 kHz,
                                                //  2: 500 kHz,
                                                //  3: Reserved]
#define LORA_SPREADING_FACTOR         7         // [SF7..SF12]
#define LORA_CODINGRATE               1         // [1: 4/5,
                                                //  2: 4/6,
                                                //  3: 4/7,
                                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH          8         // Same for Tx and Rx

#define TX_OUTPUT_POWER               14        // dBm
#define LORA_SYMBOL_TIMEOUT           0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON    false
#define LORA_IQ_INVERSION_ON          false


// CubeCell setup code (include in sketch)

//  Radio.SetChannel( RF_FREQUENCY );
//  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
//                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
//                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
//                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

// Rationalised Parameter Definitions (See above for CubeCell options)
// Use only these definitions in future, the above to be deprecated at some point

#define Frequency				917E6
#define SignalBandwidth			125E3
#define	SignalBandwidthIndex	0
#define SpreadingFactor			7
#define CodingRate				1
#define CodingRateDenominator	5
#define PreambleLength			8
#define SyncWord				0x12
#define OutputPower				14
#define SymbolTimeout			0
#define FixedLengthPayload		false
#define IQInversion				false

// ESP-LoRa setup code (Sandeep Mistry LoRa library)

// #define CS 18
// #define RST 14
// #define DI0 26
//
//  SPI.begin(5,19,27,18);
//  LoRa.setPins(CS,RST,DI0);
//  if (!LoRa.begin(Frequency)) {
//    Serial.println("[setup] Starting LoRa failed!");
//    while (true);
//  }
//  LoRa.setSpreadingFactor(SpreadingFactor);
//  LoRa.setSignalBandwidth(SignalBandwidth);
//  LoRa.setPreambleLength(PreambleLength);
//  LoRa.setCodingRate4(CodingRateDenominator);
//  LoRa.setSyncWord(SyncWord);
//  LoRa.disableCrc();

// CubeCell setup code (Heltec LoRa_APP or LoRaWan_APP library)

//  Radio.SetChannel( Frequency );
//  Radio.SetTxConfig( MODEM_LORA, OutputPower, 0, SignalBandwidthIndex,
//                                 SpreadingFactor, CodingRate,
//                                 PreambleLength, FixedLengthPayload,
//                                 true, 0, 0, IQInversion, 3000 );

#endif