/*
    PacketHandler - Langlo data packet management
    Version 0.0.22	15/01/2024

    0.0.1		Initial release
    0.0.2 	Add setContent() function
    0.0.3 	Augment Software Revision function
    0.0.4 	Add EepromHandler library usage
		0.0.5 	Amend hexDump() totalByteCount to _totalByteCount
		0.0.6 	Add PUMP packet details
						Expand TANK type to include Tank ID
		0.0.7 	Add ACK packet type
						Reorder PH_packetType and rearrange the declaration/definition of
						PH_packetID / _packetID[]
		0.0.8 	Add ACK bit to packet structure (obviating the need for an ACK packet!)
						Add packetTypeDescription() function
		0.0.9 	Change all const floor/ceiling to static const (for LinkedList compatibility)
						Change all const float floor/ceiling to static constexpr float
		0.0.10	Add Destination MAC and name to serialOut()
		0.0.11	Suppress heartbeat topic for ACK packets (mqttOut())
		0.0.12	Add REL bit to packet structure to signal request for acknowledgement
						Replace ACK bit with REL bit and move ACK bit to the top of the Length field
						Add/modify relevant get and set functions
						Modify serialOut() so that ACK packets do not display any content (because
						there isn't any)
		0.0.13	Add setAckFlag() option to leave payload length untouched
		0.0.14	Remove references to the EEPROM Handler
						Move MQTT topic information to .h file
		0.0.15	Remove F() macro usage (in .cpp) when printing _packetID[_typeIndex].description
						(curiously, caused compilation error with Pro Mini and ESP8266, but not ESP32
						or ASR650x) 
		0.0.16	Amend declaration of [pack] TANK packet struct (__attribute__ ((__packed__)))
		        I'm not sure why this isn't required on all structs that include an 8-bit
		        field - maybe it is and I just haven't run into the problem elsewhere...
		0.0.17	Add Water Flow packet type
		0.0.18	Expand Pump packet type to include Operating Mode
		        Change PH_PowerStatus to PH_PowerState
		        Change [set]pumpPowerState() to [set]powerState()
		0.0.19	Add POWER_FAIL to PH_PowerState type
		0.0.20	Add erasePacketPayload() to begin(... _type)
		0.0.21	Amend PH_CONTROL_MODE entity names
		0.0.22	Add PH_SwitchState type and rationalise with PH_PowerState (cf. PUMP packet)
		
    Digital Concepts
    15 Jan 2024
    digitalconcepts.net.au
 */

#ifndef PacketHandler_h
#define PacketHandler_h

#include <Arduino.h>
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
  #include <SSD1306.h>        // For ESP8266/ESP32 OLED display
  #include <PubSubClient.h>   // MQTT, only applicable for the WiFi-enabled nodes
#elif defined(__ASR_Arduino__)
  #include <HT_SH1107Wire.h>  // For Heltec CubeCell Plus OLED display
#endif
#include <CRC32.h>
#include <NodeHandler.h>

enum PH_handler {
	PACKET_HANDLER = 0,
	NODE_HANDLER
};

// Index to packet Types
// enum takes int-size memory by default, so specify that it's just one byte
// Not critical for PH_packetType, but it is for PH_powerStatus

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
  FLOW,             // 18
  NUM_TYPES
} PH_packetType;

typedef enum: uint8_t {
	PH_POWER_FAIL = 0,
	PH_POWER_LIVE,
	PH_POWER_UNDEFINED,
  NUM_SUPPLIES
} PH_powerSupply;

typedef enum: uint8_t {
	PH_RELAY_OFF = 0,
	PH_RELAY_ON,
	PH_RELAY_UNDEFINED,
  NUM_STATES
} PH_relayState;

typedef enum: uint8_t {
	PH_MODE_REMOTE = 0,
	PH_MODE_LOCAL,
	PH_MODE_UNDEFINED,
  NUM_MODES
} PH_controlMode;

class PacketHandler {

  private:

    // Need to change "Size" to "ByteCount" in all the following variables to more clearly
    // indicate what they represent

    // Langlo LoRa packet structure

    static const uint8_t PH_packetSize = 64;
    static const uint8_t PH_headerSize = 16;
    static const uint8_t PH_genericPayloadSize = PH_packetSize - PH_headerSize;

    // ACK Type

    static const int PH_ackPayloadSize = 0;

    // Voltage Type

    static const int PH_voltagePayloadSize = 2;
    union PH_voltagePayload {
      uint8_t payloadByte[PH_voltagePayloadSize];
      struct payloadContent {
        uint16_t voltage;					// mV
      } source;
    };

    static const uint16_t PH_voltageFloor = 0;
    static const uint16_t PH_voltageCeiling = 12000;

    // Power Type

    static const int PH_powerPayloadSize = 12;
    union PH_powerPayload {
      uint8_t payloadByte[PH_powerPayloadSize];
      struct payloadContent {
        uint16_t voltage;					// mV
        uint16_t current;					// mA
      } battery, panel, load;
    };

    static const uint16_t PH_batteryVoltageFloor = 0;
    static const uint16_t PH_batteryVoltageCeiling = 12000;

    static const uint16_t PH_panelVoltageFloor = 0;
    static const uint16_t PH_panelVoltageCeiling = 12000;

    static const uint16_t PH_loadVoltageFloor = 0;
    static const uint16_t PH_loadVoltageCeiling = 12000;

    static const uint16_t PH_batteryCurrentFloor = 0;
    static const uint16_t PH_batteryCurrentCeiling = 1000;

    static const uint16_t PH_panelCurrentFloor = 0;
    static const uint16_t PH_panelCurrentCeiling = 1000;

    static const uint16_t PH_loadCurrentFloor = 0;
    static const uint16_t PH_loadCurrentCeiling = 1000;

    // Tank Type

    static const int PH_tankPayloadSize = 3;
    union PH_tankPayload {
      uint8_t payloadByte[PH_tankPayloadSize];
			struct __attribute__ ((__packed__)) payloadContent {
        uint8_t id;							// Tank ID
        uint16_t level;					// Distance to water level (mm)
      } tank;
    };

    static const uint16_t PH_tankLevelFloor = 0;
    static const uint16_t PH_tankLevelCeiling = 100;

    // Pump Type

    static const int PH_pumpPayloadSize = 4;
    union PH_pumpPayload {
      uint8_t payloadByte[PH_pumpPayloadSize];
      struct payloadContent {
        uint8_t id;							// Pump ID
        PH_powerSupply power;		// Power state
        PH_relayState relay;	  // Relay state
        PH_controlMode mode;		// Controller operating mode
      } pump;
    };

    // Weather Type

    static const int PH_weatherPayloadSize = 11;
    union PH_weatherPayload {
      uint8_t payloadByte[PH_weatherPayloadSize];
      struct payloadContent {
        int16_t temperature;				// (Temperature x 10) °C
        uint16_t pressure;					// kPa
        uint16_t humidity;					// %
        uint16_t rainfall;					// mm
        uint16_t windBearing;				// °
        uint8_t windSpeed;					// kph
      } recorded;
    };

    static const int16_t PH_temperatureFloor = -500;
    static const int16_t PH_temperatureCeiling = 1000;

    static const uint16_t PH_pressureFloor = 800;
    static const uint16_t PH_pressureCeiling = 1100;

    static const uint16_t PH_humidityFloor = 0;
    static const uint16_t PH_humidityCeiling = 100;

    static const uint16_t PH_rainfallFloor = 0;
    static const uint16_t PH_rainfallCeiling = 1000;

    static const uint16_t PH_windBearingFloor = 0;
    static const uint16_t PH_windBearingCeiling = 360;

    static const uint8_t PH_windSpeedFloor = 0;
    static const uint8_t PH_windSpeedCeiling = 250;

    // Atmosphere Type

    static const int PH_atmospherePayloadSize = 6;
    union PH_atmospherePayload {
      uint8_t payloadByte[PH_atmospherePayloadSize];
      struct payloadContent {
        int16_t temperature;
        uint16_t pressure;
        uint16_t humidity;
      } recorded;
    };

    // Temperature Type

    static const int PH_temperaturePayloadSize = 2;
    union PH_temperaturePayload {
      uint8_t payloadByte[PH_temperaturePayloadSize];
      struct payloadContent {
        int16_t temperature;
      } recorded;
    };

    // Rainfall Type

    static const int PH_rainfallPayloadSize = 2;
    union PH_rainfallPayload {
      uint8_t payloadByte[PH_rainfallPayloadSize];
      struct payloadContent {
        uint16_t rainfall;
      } recorded;
    };

    // Wind Type

    static const int PH_windPayloadSize = 3;
    union PH_windPayload {
      uint8_t payloadByte[PH_windPayloadSize];
      struct payloadContent {
        uint16_t windBearing;
        uint8_t windSpeed;
      } recorded;
    };

    // VOX Type

    static const int PH_voxPayloadSize = 2;
    union PH_voxPayload {
      uint8_t payloadByte[PH_voxPayloadSize];
      struct payloadContent {
        uint16_t voxLevel;					// ?
      } recorded;
    };

    static const uint16_t PH_voxFloor = 0;
    static const uint16_t PH_voxCeiling = 1000;

    // Light Type

    static const int PH_lightPayloadSize = 2;
    union PH_lightPayload {
      uint8_t payloadByte[PH_lightPayloadSize];
      struct payloadContent {
        uint16_t ambientLight;				// ? x 100
      } recorded;
    };

    static const uint16_t PH_lightFloor = 0;
    static const uint16_t PH_lightCeiling = 1000;

    // UV Type

    static const int PH_uvPayloadSize = 6;
    union PH_uvPayload {
      uint8_t payloadByte[PH_uvPayloadSize];
      struct payloadContent {
        uint16_t uvA;						// ?
        uint16_t uvB;						// ?
        uint16_t uvIndex;					// ?
      } recorded;
    };

    static const uint16_t PH_uvAFloor = 0;
    static const uint16_t PH_uvACeiling = 1000;

    static const uint16_t PH_uvBFloor = 0;
    static const uint16_t PH_uvBCeiling = 1000;

    static const uint16_t PH_uvIndexFloor = 0;
    static const uint16_t PH_uvIndexCeiling = 1000;

    // Controller Type

    static const int PH_sprinklerPayloadSize = 48;
    union PH_sprinklerPayload {
      uint8_t payloadByte[PH_sprinklerPayloadSize];	// No structure defined at this time
    };

    // GPS Type

    static const int PH_gpsPayloadSize = 16;
    union PH_gpsPayload {
      uint8_t payloadByte[PH_gpsPayloadSize];
      struct payloadContent {
        float latitude;						// °
        float longitude;					// °
        float altitude;						// m
        float hdop;							// ?
      } position;
    };

    static constexpr float PH_latitudeFloor = -90.0;
    static constexpr float PH_latitudeCeiling = 90.0;

    static constexpr float PH_longitudeFloor = -180.0;
    static constexpr float PH_longitudeCeiling = 180.0;

    static constexpr float PH_altitudeFloor = 0.0;
    static constexpr float PH_altitudeCeiling = 10000.0;

    // AWTS Type

    static const int PH_awtsPayloadSize = 3;
    union PH_awtsPayload {
      uint8_t payloadByte[PH_awtsPayloadSize];
      struct payloadContent {
        uint16_t blowerPressure;			// Pa
        uint8_t itLevel;					// % of full
      } recorded;
    };

    static const uint16_t PH_blowerPressureFloor = 0;
    static const uint16_t PH_blowerPressureCeiling = 2000;

    static const uint8_t PH_itLevelFloor = 0;
    static const uint8_t PH_itLevelCeiling = 100;

    // Reset Type

    static const int PH_resetPayloadSize = 1;
    union PH_resetPayload {
      uint8_t payloadByte[PH_resetPayloadSize];
      struct payloadContent {
        uint8_t code;							// Reset flag
      } error;
    };

    // Flow Type

    static const int PH_flowPayloadSize = 4;
    union PH_flowPayload {
      uint8_t payloadByte[PH_flowPayloadSize];
      struct payloadContent {
        uint16_t rate;							// Flow Rate
        uint16_t volume;					  // Flow Volume
      } recorded;
    };

    static const uint16_t PH_flowRateFloor = 0;
    static const uint16_t PH_flowRateCeiling = 65535;

    // Generic Type

    union PH_genericPayload {
      uint8_t payload[PH_genericPayloadSize];
      PH_voltagePayload voltage;
      PH_powerPayload power;
      PH_tankPayload tank;
      PH_pumpPayload pump;
      PH_weatherPayload weather;
      PH_atmospherePayload atmosphere;
      PH_temperaturePayload temperature;
      PH_rainfallPayload rainfall;
      PH_windPayload wind;
      PH_voxPayload vox;
      PH_lightPayload light;
      PH_uvPayload uv;
      PH_sprinklerPayload sprinkler;
      PH_gpsPayload gps;
      PH_awtsPayload awts;
      PH_resetPayload reset;
      PH_flowPayload flow;
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

// content.packetType includes top bit as REL flag and lower 7 bits as Type code
// content.byteCount includes top bit as ACK flag and lower 7 bits as payload byte count

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
      {DEFAULT_TYPE,	0x7F, PH_genericPayloadSize,			"DEFAULT_TYPE"  },
      {FLOW,	        0x13, PH_flowPayloadSize,			    "FLOW"          }
    };

// Index to sensor MQTT topics

    enum topicList {
      T_HEARTBEAT = 0,    //  0
      T_VOLTAGE,          //  1
      T_BATTERY_VOLTAGE,  //  2
      T_BATTERY_CURRENT,  //  3
      T_PANEL_VOLTAGE,    //  4
      T_PANEL_CURRENT,    //  5
      T_LOAD_VOLTAGE,     //  6
      T_LOAD_CURRENT,     //  7
      T_TANK_LEVEL,       //  8
      T_PUMP_POWER,       //  9
      T_PUMP_RELAY,       // 10
      T_PUMP_MODE,        // 11
      T_TEMPERATURE,      // 12
      T_HUMIDITY,         // 13
      T_PRESSURE,         // 14
      T_RAINFALL,         // 15
      T_WIND_BEARING,     // 16
      T_WIND_SPEED,       // 17
      T_VOX,              // 18
      T_AMBIENT_LIGHT,    // 19
      T_UV_A,             // 20
      T_UV_B,             // 21
      T_UV_INDEX,         // 22
      T_LATITUDE,         // 23
      T_LONGITUDE,        // 24
      T_ALTITUDE,         // 25
      T_AWTS_PRESSURE,    // 26
      T_AWTS_LEVEL,       // 27
      T_RESET_CODE,       // 28
      T_FLOW_RATE,        // 29
      T_FLOW_VOLUME,      // 30
      T_NUM_TOPICS
    };

// MQTT sensor topics (entries matched to above index)

    static constexpr char* SENSOR_TOPIC[T_NUM_TOPICS] = {
      "heartbeat",       //  0
      "voltage",         //  1
      "batteryvoltage",  //  2
      "batterycurrent",  //  3
      "panelvoltage",    //  4
      "panelcurrent",    //  5
      "loadvoltage",     //  6
      "loadcurrent",     //  7
      "tanklevel",			 //  8
      "powersupply",     //  9
      "relaystate",      // 10
      "controlmode",     // 11
      "temperature",     // 12
      "humidity",        // 13
      "pressure",        // 14
      "rainfall",        // 15
      "windbearing",     // 16
      "windspeed",       // 17
      "vox",             // 18
      "light",           // 19
      "uva",             // 20
      "uvb",             // 21
      "uvindex",         // 22
      "latitude",        // 23
      "longitude",       // 24
      "altitude",        // 25
      "blowerpressure",  // 26
      "itlevel",         // 27
      "reset",           // 28
      "flowrate",        // 29
      "flowvolume"       // 30
		};

    static const int PH_topicBufferSize = 50;    // The size of the MQTT topic buffer (must be at least as long as the longest topic)

    PH_dataPacket _packet;
    PH_dataPacket *_packetPtr;
    PH_genericPayload *_payloadPtr;
    NodeHandler _node;
    
    int typeIndex(int _type);
    void erasePacketPayload();
    uint32_t calculatePayloadChecksum();
    
  public:
  
    PacketHandler();
    
    String softwareRevision();
		String softwareRevision(PH_handler _package);

    void init();
    void begin();
    void begin(uint32_t _destinationMAC);
    void begin(uint32_t _destinationMAC, uint32_t _sourceMAC);
    void begin(uint32_t _destinationMAC, uint32_t _sourceMAC, uint16_t _sequenceNumber);
    void begin(uint32_t _destinationMAC, uint32_t _sourceMAC, uint16_t _sequenceNumber, PH_packetType _type);
/*
    void setNodeAttributes(uint32_t _mac);
    void setNodeAttributes(uint32_t _mac, uint16_t _timer);
    void setNodeAttributes(uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue);
    void setNodeAttributes(uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue, char * _name);
    */
    void setNodeAttributes(uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue, const char * _name, const char * _topic);

	  uint16_t  heartbeatInterval();
	  uint16_t  healthCheckInterval();
	  void incrementNodeTimers();
  #if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
	  void nodeHealthCheck(PubSubClient *_clientPtr);
  #else
	  void nodeHealthCheck();
  #endif
  
    uint8_t   packetSize();
    uint8_t   headerSize();
    uint8_t   headerByteCount();

    uint32_t  destinationMAC();
    uint32_t  sourceMAC();
    uint32_t	checksum();
    uint16_t	sequenceNumber();
    PH_packetType		packetType();
    const char*     packetTypeDescription();
    bool      relFlag();
    bool      ackFlag();
    uint8_t		byteCount();
    uint8_t		byte(uint8_t i);
    uint8_t   *byteStream();
	  uint8_t		payloadByteCount();
	  uint8_t		packetByteCount();

    uint16_t  voltage();

    uint16_t  batteryVoltage();
    uint16_t  batteryCurrent();
    uint16_t  panelVoltage();
    uint16_t  panelCurrent();
    uint16_t  loadVoltage();
    uint16_t  loadCurrent();

    uint8_t   tankId();
    uint16_t  tankLevel();

    uint8_t   pumpId();
    PH_powerSupply   powerSupply();
    PH_relayState    relayState();
    PH_controlMode   controlMode();

    uint16_t  flowRate();
    uint16_t  flowVolume();

    int16_t   temperature();
    uint16_t  pressure();
    uint16_t  humidity();
    uint16_t  rainfall();
    uint16_t  windBearing();
    uint8_t   windSpeed();

    uint16_t  voxLevel();

    uint16_t  ambientLight();

    uint16_t  uvA();
    uint16_t  uvB();
    uint16_t  uvIndex();

    float     latitude();
    float     longitude();
    float     altitude();
    float     hdop();
    
    uint16_t  blowerPressure();
    uint8_t   itLevel();

    uint8_t   resetCode();

    void erasePacketHeader();

	  void setDestinationMAC(uint32_t _destinationMAC);
	  void setSourceMAC(uint32_t _sourceMAC);
    void setSequenceNumber(uint16_t _sequenceNumber);
    void setPacketType(PH_packetType _type);
    void setRelFlag();
    void setAckFlag();
    void setAckFlag(bool _zero);
    void setByte(uint8_t _i, uint8_t _byte);
		void setContent(uint8_t *_payload, uint16_t _byteCount);
    
	  void setVoltage(uint16_t _voltage);

	  void setBatteryVoltage(uint16_t _batteryVoltage);
	  void setBatteryCurrent(uint16_t _batteryCurrent);
	  void setPanelVoltage(uint16_t _panelVoltage);
	  void setPanelCurrent(uint16_t _panelCurrent);
	  void setLoadVoltage(uint16_t _loadVoltage);
	  void setLoadCurrent(uint16_t _loadCurrent);
	  
	  void setTankId(uint8_t _tankId);
	  void setTankLevel(uint16_t _tankLevel);

	  void setPumpId(uint8_t _pumpId);
	  void setPowerSupply(PH_powerSupply _powerSupply);
	  void setRelayState(PH_relayState _relayState);
	  void setControlMode(PH_controlMode _controlMode);

	  void setFlowRate(uint16_t _flowRate);
	  void setFlowVolume(uint16_t _flowVolume);
	  
	  void setTemperature(int16_t _temperature);
	  void setPressure(uint16_t _pressure);
	  void setHumidity(uint16_t _humidity);
	  void setRainfall(uint16_t _rainCount);
	  void setWindBearing(uint16_t _windBearing);
	  void setWindSpeed(uint8_t _windSpeed);

	  void setVoxLevel(uint16_t _voxLevel);

	  void setAmbientLight(uint16_t _ambientLightLevel);

	  void setUvA(uint16_t _uva);
	  void setUvB(uint16_t _uvb);
	  void setUvIndex(uint16_t _uvIndex);

	  void setLatitude(float _latitude);
	  void setLongitude(float _longitude);
	  void setAltitude(float _altitude);
	  void setHdop(float _hdop);
	  
	  void setBlowerPressure(int16_t _blowerPressure);
	  void setItLevel(uint16_t _itLevel);
	  
	  void setResetCode(uint8_t _resetCode);
	  
    void generatePayloadChecksum();
    bool verifyPayloadChecksum();
    
    void hexDump();
    void serialOut();
  #if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    void displayOut(SSD1306 *_displayPtr); // For ESP8266/ESP32
    void mqttOut(PubSubClient *_clientPtr);
  #elif defined(__ASR_Arduino__)
    void displayOut(SH1107Wire *_displayPtr); // For Heltec CubeCell Plus
  #endif
};

#endif