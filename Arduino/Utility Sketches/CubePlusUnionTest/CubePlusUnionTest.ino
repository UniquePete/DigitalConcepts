#include <Arduino.h>

typedef enum: uint8_t {
  ACK = 0,          //  0
  VOLTAGE,          //  1
  POWER,            //  2
  TANK,             //  3
  PUMP,             //  4
  WEATHER,          //  5
  ATMOSPHERE,       //  6
  TEMPERATURE,      //  7
  RAINFALL,         //  8
  WIND,             //  9
  VOX,              // 10
  LIGHT,            // 11
  UV,               // 12
  SPRINKLER,        // 13
  GPS,              // 14
  AWTS,             // 15
  RESET,            // 16
  DEFAULT_TYPE,     // 17
  NUM_TYPES
} PH_packetType;

// Langlo LoRa packet structure

static const uint8_t PH_packetSize = 64;
static const uint8_t PH_headerSize = 16;
static const uint8_t PH_genericPayloadSize = PH_packetSize - PH_headerSize;

static const int PH_ackPayloadSize = 0;
static const int PH_voltagePayloadSize = 2;
static const int PH_powerPayloadSize = 12;
static const int PH_pumpPayloadSize = 2;
static const int PH_weatherPayloadSize = 11;
static const int PH_atmospherePayloadSize = 6;
static const int PH_temperaturePayloadSize = 2;
static const int PH_rainfallPayloadSize = 2;
static const int PH_windPayloadSize = 3;
static const int PH_voxPayloadSize = 2;
static const int PH_lightPayloadSize = 2;
static const int PH_uvPayloadSize = 6;
static const int PH_sprinklerPayloadSize = 48;
static const int PH_gpsPayloadSize = 16;
static const int PH_awtsPayloadSize = 3;
static const int PH_resetPayloadSize = 1;

// Tank Type

static const int PH_tankPayloadSize = 3;
union PH_tankPayload {
  uint8_t payloadByte[PH_tankPayloadSize];
  struct __attribute__ ((__packed__)) payloadContent {
    uint8_t id;							// Tank ID
    uint16_t level;					// % full
  } tank;
};

// Generic Type

union PH_genericPayload {
  uint8_t payload[PH_genericPayloadSize];
  PH_tankPayload tank;
};

union PH_dataPacket {
  uint8_t packetByte[PH_packetSize];
  struct packetContent {
    uint32_t destinationMAC;        	// 4 bytes
    uint32_t sourceMAC;             	// 4 bytes
    uint32_t checksum;              	// 4 bytes
    uint16_t sequenceNumber;        	// 2 bytes
    uint8_t packetType;               // 1 byte
    uint8_t byteCount;              	// 1 byte
    PH_genericPayload packetPayload;	// max 48 bytes
  } content;
};

static const uint8_t _relMask = 0x80;
static const uint8_t _typeMask = 0x7F;
static const uint8_t _ackMask = 0x80;
static const uint8_t _lengthMask = 0x7F;

struct PH_packetID {
  const int index;
  const int type;
  const int payloadBytes;
  const char* description;
};

// Note that a static member requires both this declaration and a definition in the .cpp
// if it is to be used outside the scope of the class in which it has been defined.

static constexpr PH_packetID _packetID[NUM_TYPES] = {
  {ACK,     			0x00, PH_ackPayloadSize, 					"ACK"       		},
  {VOLTAGE, 			0x01, PH_voltagePayloadSize,			"VOLTAGE"       },
  {POWER,   			0x02, PH_powerPayloadSize, 				"POWER"         },
  {TANK,    			0x11, PH_tankPayloadSize,					"TANK"          },
  {PUMP,    			0x12, PH_pumpPayloadSize,					"PUMP"          },
  {WEATHER,				0x20, PH_weatherPayloadSize,			"WEATHER"       },
  {ATMOSPHERE,		0x21, PH_atmospherePayloadSize,		"ATMOSPHERE"    },
  {TEMPERATURE,   0x27, PH_temperaturePayloadSize,	"TEMPERATURE"   },
  {RAINFALL,			0x22, PH_rainfallPayloadSize,			"RAINFALL"      },
  {WIND,					0x23, PH_windPayloadSize,					"WIND"          },
  {VOX,						0x24, PH_voxPayloadSize,					"VOX"           },
  {LIGHT,					0x25, PH_lightPayloadSize,				"LIGHT"         },
  {UV,						0x26, PH_uvPayloadSize,						"UV"            },
  {SPRINKLER,			0x30, PH_sprinklerPayloadSize,		"SPRINKLER"     },
  {GPS,						0x40, PH_gpsPayloadSize,					"GPS"           },
  {AWTS,					0x50, PH_awtsPayloadSize,					"AWTS"          },
  {RESET,					0x7E, PH_resetPayloadSize,				"RESET"         },
  {DEFAULT_TYPE,	0x7F, PH_genericPayloadSize,			"DEFAULT_TYPE"  }
};

PH_dataPacket _packet;
PH_dataPacket *_packetPtr;
PH_genericPayload *_payloadPtr;

uint8_t ut_tankID = 1;
uint16_t ut_tankLevel = 72;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("[setup] CubeCell Plus Union Test");
  Serial.println("[setup] Initial packet state");
  hexDump();
  Serial.println("[setup] Set type to TANK");
  setPacketType( TANK );
  hexDump();
  Serial.println("[setup] Set last four bytes to 0xFF");
  _packet.packetByte[16] =0xFF;
  _packet.packetByte[17] =0xFF;
  _packet.packetByte[18] =0xFF;
  _packet.packetByte[19] =0xFF;
  hexDump();
  Serial.println("[setup] Set tankID");
  setTankId( ut_tankID );
  hexDump();
  Serial.println("[setup] Set tankLevel");
  setTankLevel( ut_tankLevel );
  hexDump();
}

void loop() {
}

uint8_t packetByteCount() {
  return (PH_headerSize + (_packet.content.byteCount & _lengthMask));
}

void setPacketType(PH_packetType _type) {
	_packet.content.packetType = _packetID[_type].type;
	_packet.content.byteCount = _packetID[_type].payloadBytes;
}

//uint8_t   tankId();
//uint16_t  tankLevel();

void setTankId(uint8_t _tankId) {
	switch (_packet.content.packetType) {
    case _packetID[TANK].type:
    {
			_packet.content.packetPayload.tank.tank.id = _tankId;
    	break;
    }
    default:
    {
      Serial.print(F("[setTankId] WARNING: No tankId element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void setTankLevel(uint16_t _tankLevel) {
	switch (_packet.content.packetType) {
    case _packetID[TANK].type:
    {
			_packet.content.packetPayload.tank.tank.level = _tankLevel;
    	break;
    }
    default:
    {
      Serial.print(F("[setTankLevel] WARNING: No tankLevel element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void hexDump() {
//  int _totalByteCount = packetByteCount() & _lengthMask;
  int _totalByteCount = 1 + ( packetByteCount() & _lengthMask );
  Serial.print(F("[hexDump]   Bytes: "));
  Serial.println( _totalByteCount );
  Serial.print(F("           Packet: "));
  for (int i = 0; i < _totalByteCount; i++) {
//    Serial.printf( "%02X", _packet.packetByte[i]);
		if (_packet.packetByte[i] < 0x10) Serial.print("0");
    Serial.print(_packet.packetByte[i],HEX);
    if ((i % 4) == 3) {
      Serial.println();
      if (i < (_totalByteCount-1)) Serial.print(F("                   "));
    } else {
      Serial.print("  ");
    }
  }
  if ((_totalByteCount % 4) != 0) Serial.println();
}
