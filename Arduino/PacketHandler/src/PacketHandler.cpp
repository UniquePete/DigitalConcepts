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

#include "PacketHandler.h"

/*
 * Note that the F() compiler macro is used extensively herein to instruct the compiler to
 * store string constants in flash memory (PROGMEM), rather than in SRAM where they would
 * otherwise be stored. This is a particular issue with the Arduino Pro Mini, which only
 * has 2KB of SRAM.
*/

// Note that a static member requires both a declaration in the .h file and a
// definition here that includes the scope (PacketHandler::) within which the relevant
// entities have been declared.

constexpr PacketHandler::PH_packetID PacketHandler::_packetID[];
constexpr char* PacketHandler::SENSOR_TOPIC[];

// Software Revision Level

#ifndef SoftwareRevision_def
  #define SoftwareRevision_def
	struct _SoftwareRevision {
		const uint8_t major;			// Major feature release
		const uint8_t minor;			// Minor feature enhancement release
		const uint8_t minimus;	  // Bug fix release
	};
#endif

_SoftwareRevision _packetHandlerRevNumber = {0,0,22};

int PacketHandler::typeIndex(int _type) {  
  for (int i = 0; i < NUM_TYPES; i++) {
    if ( _packetID[i].type == _type ) return i;
  }
  return NUM_TYPES;
}

PacketHandler::PacketHandler() {
}
    
String PacketHandler::softwareRevision() {
	String _softwareRev;
	_softwareRev = String(_packetHandlerRevNumber.major) + "." + String(_packetHandlerRevNumber.minor) + "." + String(_packetHandlerRevNumber.minimus);
	_softwareRev = _softwareRev + " - " + _node.softwareRevision();
	return _softwareRev;
}
    
String PacketHandler::softwareRevision(PH_handler _package) {
	String _softwareRev;
	switch ( _package ) {
		case PACKET_HANDLER: {
			_softwareRev = String(_packetHandlerRevNumber.major) + "." + String(_packetHandlerRevNumber.minor) + "." + String(_packetHandlerRevNumber.minimus);
			break;
		}
		case NODE_HANDLER: {
			_softwareRev = _node.softwareRevision();
			break;
		}
	}
	return _softwareRev;
}

void PacketHandler::init() {
  _node.init();
	erasePacketHeader();
	_packet.content.packetType = _packetID[DEFAULT_TYPE].type;
	_packet.content.byteCount = _packetID[DEFAULT_TYPE].payloadBytes;
	erasePacketPayload();
}

void PacketHandler::begin() {
  init();
}

void PacketHandler::begin(uint32_t _destinationMAC) {
  init();
	_packet.content.destinationMAC = _destinationMAC;
}

void PacketHandler::begin(uint32_t _destinationMAC, uint32_t _sourceMAC) {
  init();
	_packet.content.destinationMAC = _destinationMAC;
	_packet.content.sourceMAC = _sourceMAC;
}

void PacketHandler::begin(uint32_t _destinationMAC, uint32_t _sourceMAC, uint16_t _sequenceNumber) {
  init();
	_packet.content.destinationMAC = _destinationMAC;
	_packet.content.sourceMAC = _sourceMAC;
	_packet.content.sequenceNumber = _sequenceNumber;
}

void PacketHandler::begin(uint32_t _destinationMAC, uint32_t _sourceMAC, uint16_t _sequenceNumber, PH_packetType _type) {
  init();
	_packet.content.destinationMAC = _destinationMAC;
	_packet.content.sourceMAC = _sourceMAC;
	_packet.content.sequenceNumber = _sequenceNumber;
	_packet.content.packetType = _packetID[_type].type;
	_packet.content.byteCount = _packetID[_type].payloadBytes;
	erasePacketPayload();
}
/*
void PacketHandler::setNodeAttributes(const uint32_t _mac) {
	_node.setNodeMAC(_mac);
}

void PacketHandler::setNodeAttributes(const uint32_t _mac, uint16_t _timer) {
	_node.setNodeMAC(_mac);
	_node.setNodeTimer(_timer);
}

void PacketHandler::setNodeAttributes(const uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue) {
	_node.setNodeMAC(_mac);
	_node.setNodeTimer(_timer);
	_node.setNodeTimerResetValue(_timerResetValue);
}

void PacketHandler::setNodeAttributes(const uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue, const char * _name) {
	_node.setNodeMAC(_mac);
	_node.setNodeTimer(_timer);
	_node.setNodeTimerResetValue(_timerResetValue);
	_node.setNodeName(_name);
}
*/
void PacketHandler::setNodeAttributes(uint32_t _mac, uint16_t _timer, uint16_t _timerResetValue, const char * _name, const char * _topic) {
	_node.setNodeMAC(_mac);
	_node.setNodeTimer(_timer);
	_node.setNodeTimerResetValue(_timerResetValue);
	_node.setNodeName(_name);
	_node.setNodeTopic(_topic);
}

void PacketHandler::setPacketType(PH_packetType _type) {
	_packet.content.packetType = _packetID[_type].type;
	_packet.content.byteCount = _packetID[_type].payloadBytes;
	erasePacketPayload();
}

void PacketHandler::erasePacketHeader() {
  int _headerStart = 0;
  int _headerEnd = PH_headerSize;
  for (int i = _headerStart; i < _headerEnd; i++) {
    _packet.packetByte[i] = 0x0;
  }
}

void PacketHandler::erasePacketPayload() {
  int _payloadStart = PH_headerSize;
  int _payloadEnd = PH_packetSize;
  for (int i = _payloadStart; i < _payloadEnd; i++) {
    _packet.packetByte[i] = 0x0;
  }
}

uint16_t PacketHandler::heartbeatInterval() {
  return _node.heartbeatInterval();
}

uint16_t PacketHandler::healthCheckInterval() {
  return _node.healthCheckInterval();
}

void PacketHandler::incrementNodeTimers() {
  for (int i = 0; i < _node.nodeCount(); i++) {
    _node.incrementTimer(i);
  }
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void PacketHandler::nodeHealthCheck(PubSubClient *_clientPtr) {
#else
void PacketHandler::nodeHealthCheck() {
#endif 
  char _topicBuffer[PH_topicBufferSize];
  String _localString;
   
  // Loop through the Node List and check individual heartbeats
/*
  Serial.print("[healthCheck] Dead Timer: ");
  Serial.println(_node.deadbeat());
*/
  for (int i = 0; i < _node.nodeCount(); i++) {
    if ( _node.timer(i) >= _node.deadbeat() ) {
    
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
      _localString = _node.topic(i) + "/heartbeat";
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, "false");
#endif
        
      _localString = "[healthCheck] Heart failure " + _node.name(i) + " timer: ";
      Serial.print(_localString);
      Serial.println(_node.timer(i));
    } else {
      _localString = "[healthCheck] " + _node.name(i) + " Timer: ";
      Serial.print(_localString);
      Serial.println(_node.timer(i));
    }
  }
  Serial.println();
}

uint8_t PacketHandler::packetSize() {
  return PH_packetSize;
}

uint8_t PacketHandler::headerSize() {
  return PH_headerSize;
}

uint32_t PacketHandler::destinationMAC() {
  return _packet.content.destinationMAC;
}

void PacketHandler::setDestinationMAC(uint32_t _destinationMAC) {
  _packet.content.destinationMAC = _destinationMAC;
}

uint32_t PacketHandler::sourceMAC() {
  return _packet.content.sourceMAC;
}

void PacketHandler::setSourceMAC(uint32_t _sourceMAC) {
  _packet.content.sourceMAC = _sourceMAC;
}

uint16_t PacketHandler::sequenceNumber() {
  return _packet.content.sequenceNumber;
}

void PacketHandler::setSequenceNumber(uint16_t _sequenceNumber) {
  _packet.content.sequenceNumber = _sequenceNumber;
}

PH_packetType PacketHandler::packetType() {
//	Serial.println();
//	Serial.print("[PacketHandler::packetType] content.packetType: ");
//	Serial.println(_packet.content.packetType,BIN);
//	Serial.print("[PacketHandler::packetType] _typeMask: ");
//	Serial.println(_typeMask,BIN);
	uint8_t _typeBits = _packet.content.packetType & _typeMask;
//	Serial.print("[PacketHandler::packetType] _typeBits: ");
//	Serial.println(_typeBits,BIN);
//	uint8_t _ackBit = _packet.content.packetType & _ackMask;
//	Serial.print("[PacketHandler::packetType] _ackBit: ");
//	Serial.println(_ackBit,BIN);
	PH_packetType _type = (PH_packetType)typeIndex(_typeBits);
//	Serial.print("[PacketHandler::packetType] _type: ");
//	Serial.println(_type,BIN);
  return _type;
}

const char* PacketHandler::packetTypeDescription() {
	uint8_t _typeBits = _packet.content.packetType & _typeMask;
	const char* _typeDescription = _packetID[typeIndex(_typeBits)].description;
  return _typeDescription;
}

bool PacketHandler::relFlag() {
	if (( _packet.content.packetType & _relMask ) == _relMask ) {
		return true;
  } else {
		return false;
  }
}

void PacketHandler::setRelFlag() {
	_packet.content.packetType = _packet.content.packetType | _relMask;
}

bool PacketHandler::ackFlag() {
	if (( _packet.content.byteCount & _ackMask ) == _ackMask ) {
		return true;
  } else {
		return false;
  }
}

void PacketHandler::setAckFlag() {
	// By default, setAckFlag zeros the payload length
		_packet.content.byteCount = _ackMask;
}

void PacketHandler::setAckFlag(bool _zero) {
	if ( _zero ) {
		// Zero the payload length
		_packet.content.byteCount = _ackMask;
	} else {
		_packet.content.byteCount = _packet.content.byteCount | _ackMask;
	}
}

uint8_t PacketHandler::packetByteCount() {
  return (PH_headerSize + (_packet.content.byteCount & _lengthMask));
}

uint8_t PacketHandler::payloadByteCount() {
  return (_packet.content.byteCount & _lengthMask);
}

uint8_t PacketHandler::byteCount() {
  return (_packet.content.byteCount & _lengthMask);
}

uint8_t PacketHandler::byte(uint8_t _i) {
	uint8_t _limit = packetByteCount() & _lengthMask;
	if ( _i < _limit) {
		return _packet.packetByte[_i];
	} else {
		Serial.println(F("[byte] WARNING: Index "));
		Serial.print(_i);
		Serial.print(F(" exceeds packet size "));
		Serial.println(_limit);
		return 0x00;
	}
}

void PacketHandler::setByte(uint8_t _i, uint8_t _byte) {
	uint8_t _limit = PH_packetSize;
	if ( _i < _limit) {
		_packet.packetByte[_i] = _byte;
	} else {
		Serial.println(F("[setByte] WARNING: Index "));
		Serial.print(_i);
		Serial.print(F(" exceeds packet size "));
		Serial.println(_limit);
	}
}

void PacketHandler::setContent(uint8_t *_payload, uint16_t _byteCount) {
	if ( _byteCount > PH_packetSize) {
		Serial.println(F("[setContent] WARNING: Payload "));
		Serial.print(_byteCount);
		Serial.print(F(" exceeds packet size "));
		Serial.println(PH_packetSize);
		_byteCount = PH_packetSize;
	}
	memcpy(_packet.packetByte, _payload, _byteCount);
}

uint8_t* PacketHandler::byteStream() {
	generatePayloadChecksum();
	return _packet.packetByte;
}

void PacketHandler::setVoltage(uint16_t _voltage) {
	switch (_packet.content.packetType) {
    case _packetID[VOLTAGE].type:
    {
			_packet.content.packetPayload.voltage.source.voltage = _voltage;
    	break;
    }
    default:
    {
      Serial.print(F("[setVoltage] WARNING: No voltage element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setTankId(uint8_t _tankId) {
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

void PacketHandler::setTankLevel(uint16_t _tankLevel) {
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

void PacketHandler::setPumpId(uint8_t _pumpId) {
	switch ( packetType() ) {
    case PUMP:
    {
			_packet.content.packetPayload.pump.pump.id = _pumpId;
    	break;
    }
    default:
    {
      Serial.print(F("[setPumpId] WARNING: No pumpId element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

uint8_t PacketHandler::pumpId() {
	switch ( packetType() ) {
    case PUMP:
    {
			return _packet.content.packetPayload.pump.pump.id;
    	break;
    }
    default:
    {
      Serial.print(F("[pumpId] WARNING: No pumpId element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return 0;
    }
	}
}

void PacketHandler::setPowerSupply(PH_powerSupply _powerSupply) {
	switch ( packetType() ) {
    case PUMP:
    {
			_packet.content.packetPayload.pump.pump.power = _powerSupply;
    	break;
    }
    default:
    {
      Serial.print(F("[setPowerSupply] WARNING: No powerSupply element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

PH_powerSupply PacketHandler::powerSupply() {
//	Serial.println();
//	Serial.println("[PacketHandler::powerSupply] Enter switch");
	switch ( packetType() ) {
    case PUMP:
    {
//			Serial.println("[PacketHandler::powerSupply] PUMP type");
  	  // Don't know why I have to things this way, but I do...
			PH_powerSupply _val;
//			Serial.println("[PacketHandler::powerSupply] Set return value...");
//			Serial.print("[PacketHandler::powerSupply] Payload setting : 0x");
//			Serial.println(_packet.content.packetPayload.pump.pump.power,HEX);
			_val = _packet.content.packetPayload.pump.pump.power;
//			Serial.print("[PacketHandler::powerSupply] Initialise _val : 0x");
//			Serial.println(_val,HEX);
//			Serial.print("[PacketHandler::powerSupply] Mask byte : 0x");
			uint8_t _maskVal = (uint8_t)_val & 0xFF;
//			Serial.println(_maskVal,HEX);
//			Serial.print("[PacketHandler::powerSupply] Return value : 0x");
			PH_powerSupply _phMaskVal = (PH_powerSupply)_maskVal;
//			Serial.println(_phMaskVal,HEX);
			return _phMaskVal;
    	break;
    }
    default:
    {
      Serial.print(F("[powerSupply] WARNING: No powerSupply element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return PH_POWER_UNDEFINED;
    }
	}
}

void PacketHandler::setRelayState(PH_relayState _relayState) {
	switch ( packetType() ) {
    case PUMP:
    {
			_packet.content.packetPayload.pump.pump.relay = _relayState;
    	break;
    }
    default:
    {
      Serial.print(F("[setRelayState] WARNING: No relayState element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

PH_relayState PacketHandler::relayState() {
//	Serial.println();
//	Serial.println("[PacketHandler::relayState] Enter switch");
	switch ( packetType() ) {
    case PUMP:
    {
//			Serial.println("[PacketHandler::relayState] PUMP type");
  	  // Don't know why I have to things this way, but I do...
			PH_relayState _val;
//			Serial.println("[PacketHandler::relayState] Set return value...");
//			Serial.print("[PacketHandler::relayState] Payload setting : 0x");
//			Serial.println(_packet.content.packetPayload.pump.pump.relay,HEX);
			_val = _packet.content.packetPayload.pump.pump.relay;
//			Serial.print("[PacketHandler::relayState] Initialise _val : 0x");
//			Serial.println(_val,HEX);
//			Serial.print("[PacketHandler::relayState] Mask byte : 0x");
			uint8_t _maskVal = (uint8_t)_val & 0xFF;
//			Serial.println(_maskVal,HEX);
//			Serial.print("[PacketHandler::relayState] Return value : 0x");
			PH_relayState _phMaskVal = (PH_relayState)_maskVal;
//			Serial.println(_phMaskVal,HEX);
			return _phMaskVal;
    	break;
    }
    default:
    {
      Serial.print(F("[relayState] WARNING: No relayState element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return PH_RELAY_UNDEFINED;
    }
	}
}

void PacketHandler::setControlMode(PH_controlMode _controlMode) {
	switch ( packetType() ) {
    case PUMP:
    {
			_packet.content.packetPayload.pump.pump.mode = _controlMode;
    	break;
    }
    default:
    {
      Serial.print(F("[setControlMode] WARNING: No pumpControlMode element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

PH_controlMode PacketHandler::controlMode() {
//	Don't know if the following method is necessary, just copying the above
	switch ( packetType() ) {
    case PUMP:
    {
			PH_controlMode _val;
			_val = _packet.content.packetPayload.pump.pump.mode;
			uint8_t _maskVal = (uint8_t)_val & 0xFF;
			PH_controlMode _phMaskVal = (PH_controlMode)_maskVal;
			return _phMaskVal;
    	break;
    }
    default:
    {
      Serial.print(F("[controlMode] WARNING: No controlMode element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return PH_MODE_UNDEFINED;
    }
	}
}

void PacketHandler::setFlowRate(uint16_t _flowRate) {
	switch ( packetType() ) {
    case FLOW:
    {
			_packet.content.packetPayload.flow.recorded.rate = _flowRate;
    	break;
    }
    default:
    {
      Serial.print(F("[setFlowRate] WARNING: No flowRate element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

uint16_t PacketHandler::flowRate() {
	switch ( packetType() ) {
    case FLOW:
    {
			return _packet.content.packetPayload.flow.recorded.rate;
    	break;
    }
    default:
    {
      Serial.print(F("[flowRate] WARNING: No flowRate element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return 0;
    }
	}
}

void PacketHandler::setFlowVolume(uint16_t _flowVolume) {
	switch ( packetType() ) {
    case FLOW:
    {
			_packet.content.packetPayload.flow.recorded.volume = _flowVolume;
    	break;
    }
    default:
    {
      Serial.print(F("[setFlowVolume] WARNING: No flowVolume element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

uint16_t PacketHandler::flowVolume() {
	switch ( packetType() ) {
    case FLOW:
    {
			return _packet.content.packetPayload.flow.recorded.volume;
    	break;
    }
    default:
    {
      Serial.print(F("[flowVolume] WARNING: No flowRate element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
			return 0;
    }
	}
}

void PacketHandler::setTemperature(int16_t _temperature) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.temperature = _temperature;
    	break;
    }
    case _packetID[ATMOSPHERE].type:
    {
		  _packet.content.packetPayload.atmosphere.recorded.temperature = _temperature;
    	break;
    }
    case _packetID[TEMPERATURE].type:
    {
		  _packet.content.packetPayload.temperature.recorded.temperature = _temperature;
    	break;
    }
    default:
    {
      Serial.print(F("[setTemperature] WARNING: No temperature element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setPressure(uint16_t _pressure) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.pressure = _pressure;
    	break;
    }
    case _packetID[ATMOSPHERE].type:
    {
		  _packet.content.packetPayload.atmosphere.recorded.pressure = _pressure;
    	break;
    }
    default:
    {
      Serial.print(F("[setPressure] WARNING: No pressure element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setHumidity(uint16_t _humidity) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.humidity = _humidity;
    	break;
    }
    case _packetID[ATMOSPHERE].type:
    {
		  _packet.content.packetPayload.atmosphere.recorded.humidity = _humidity;
    	break;
    }
    default:
    {
      Serial.print(F("[setHumidity] WARNING: No humidity element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setRainfall(uint16_t _rainfall) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.rainfall = _rainfall;
    	break;
    }
    case _packetID[RAINFALL].type:
    {
		  _packet.content.packetPayload.rainfall.recorded.rainfall = _rainfall;
    	break;
    }
    default:
    {
      Serial.print(F("[setRainfall] WARNING: No rainfall element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setWindBearing(uint16_t _windBearing) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.windBearing = _windBearing;
    	break;
    }
    case _packetID[WIND].type:
    {
		  _packet.content.packetPayload.wind.recorded.windBearing = _windBearing;
    	break;
    }
    default:
    {
      Serial.print(F("[setWindBearing] WARNING: No windBearing element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setWindSpeed(uint8_t _windSpeed) {
	switch (_packet.content.packetType) {
    case _packetID[WEATHER].type:
    {
		  _packet.content.packetPayload.weather.recorded.windSpeed = _windSpeed;
    	break;
    }
    case _packetID[WIND].type:
    {
		  _packet.content.packetPayload.wind.recorded.windSpeed = _windSpeed;
    	break;
    }
    default:
    {
      Serial.print(F("[setWindSpeed] WARNING: No windSpeed element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setBlowerPressure(int16_t _blowerPressure) {
	switch (_packet.content.packetType) {
    case _packetID[AWTS].type:
    {
			_packet.content.packetPayload.awts.recorded.blowerPressure = _blowerPressure;
    	break;
    }
    default:
    {
      Serial.print(F("[setBlowerPressure] WARNING: No blowerPressure element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setItLevel(uint16_t _itLevel) {
	switch (_packet.content.packetType) {
    case _packetID[AWTS].type:
    {
			_packet.content.packetPayload.awts.recorded.itLevel = _itLevel;
    	break;
    }
    default:
    {
      Serial.print(F("[setItLevel] WARNING: No itLevel element for packet type 0x"));
//      Serial.printf("%02X", _packet.content.packetType);
      Serial.print(_packet.content.packetType,HEX);
      Serial.println();
    }
	}
}

void PacketHandler::setResetCode(uint8_t _resetCode) {
	_packet.content.packetPayload.reset.error.code = _resetCode;
}

uint32_t PacketHandler::calculatePayloadChecksum() {
  CRC32 crc;
  int _checksumStart = PH_headerSize - 4;
  int _checksumEnd = packetByteCount();
  for (int i = _checksumStart; i < _checksumEnd; i++) {
    crc.update(_packet.packetByte[i]);
  }
  uint32_t _localChecksum = crc.finalize();
  return _localChecksum;
}

void PacketHandler::generatePayloadChecksum() {
  _packet.content.checksum = calculatePayloadChecksum();
}

uint32_t PacketHandler::checksum() {
  return _packet.content.checksum;
}

bool PacketHandler::verifyPayloadChecksum() {
  if (calculatePayloadChecksum() == checksum()) {
    return true;
  } else {
		Serial.print(F("[verifyPayloadChecksum] Checksum: "));
		Serial.println(checksum(),HEX);
/*
		for (int _i = 8; _i < 12; _i++) {
			if (_packet.packetByte[_i] < 0x10) Serial.print("0");
			Serial.print(_packet.packetByte[_i],HEX);
			if (_i < 11) Serial.print("  ");
		}
*/
		Serial.print(F("             Calculated Checksum: "));
		Serial.println(calculatePayloadChecksum(),HEX);
/*
		uint32_t _localChecksum = calculatePayloadChecksum();
		for (int _i = 0; _i < 4; _i++) {
			if (_packet.packetByte[_i] < 0x10) Serial.print("0");
			Serial.print(_packet.packetByte[_i],HEX);
			if (_i < 2) Serial.print("  ");
		}
*/
    return false;
  }
}

void PacketHandler::hexDump() {
  int _totalByteCount = packetByteCount() & _lengthMask;
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

void PacketHandler::serialOut() {
  Serial.println(F("[serialOut] Packet Content"));

  // Extract the header info first
  
  Serial.print(F("  Destination MAC: ")); 
//  Serial.printf("%02X", _packet.content.destinationMAC);
  Serial.print(_packet.content.destinationMAC,HEX);
  Serial.println();
  
  // Get the index of the destination Node details
  
  //  NodeHandler _node;
  int _dNodeIndex = _node.index(_packet.content.destinationMAC);
  
  // And let them know who it is
  
  Serial.print(F("        Node Name: ")); 
  Serial.println(_node.name(_dNodeIndex)); 
  
  Serial.print(F("       Source MAC: ")); 
//  Serial.printf("%02X", _packet.content.sourceMAC);
  Serial.print(_packet.content.sourceMAC,HEX);
  Serial.println();
  
  // Get the index of the source Node details
  
  //  NodeHandler _node;
  int _sNodeIndex = _node.index(_packet.content.sourceMAC);
  
  // And let them know who it is
  
  Serial.print(F("        Node Name: ")); 
  Serial.println(_node.name(_sNodeIndex));
  
  Serial.print(F("  Sequence Number: ")); 
  Serial.println(_packet.content.sequenceNumber);
  Serial.print(F("         REL Flag: "));
	if ( this->relFlag() ) {
		Serial.println("True");
	} else {
		Serial.println("False");
	}
  Serial.print(F("         ACK Flag: "));
	if ( this->ackFlag() ) {
		Serial.println("True");
	} else {
		Serial.println("False");
	}
  Serial.print(F("      Packet Type: "));
  
  // Now the rest of the packet
  
  PH_genericPayload *_payloadPtr;
  _payloadPtr = &_packet.content.packetPayload;
  PH_packetType _typeIndex = this->packetType();
  
	Serial.println(_packetID[_typeIndex].description);
	
	if ( !this->ackFlag() ) {
		switch (_packet.content.packetType & _typeMask) {
			case _packetID[ACK].type:
			{
				Serial.println();
				break;
			}
			case _packetID[VOLTAGE].type:
			{
				Serial.print(F("              Vcc: "));
				Serial.print((float)_payloadPtr->voltage.source.voltage/1000, 2);
				Serial.println(F(" V"));
				break;
			}
			case _packetID[POWER].type:
			{
				Serial.print(F("  Battery Voltage: "));
				Serial.print((float)_payloadPtr->power.battery.voltage/1000, 2);
				Serial.println(F(" V"));
				Serial.print(F("  Battery Current: "));
				Serial.print(_payloadPtr->power.battery.current);
				Serial.println(F(" mA"));
				Serial.print(F("    Panel Voltage: "));
				Serial.print((float)_payloadPtr->power.panel.voltage/1000, 2);
				Serial.println(F(" V"));
				Serial.print(F("    Panel Current: "));
				Serial.print(_payloadPtr->power.panel.current);
				Serial.println(F(" mA"));
				Serial.print(F("     Load Voltage: "));
				Serial.print((float)_payloadPtr->power.load.voltage/1000, 2);
				Serial.println(F(" V"));
				Serial.print(F("     Load Voltage: "));
				Serial.print(_payloadPtr->power.load.current);
				Serial.println(F(" mA"));
				break;
			}
			case _packetID[TANK].type:
			{
				Serial.print(F("               ID: "));
				Serial.println(_payloadPtr->tank.tank.id);
				Serial.print(F("      Level Index: "));
				Serial.println(_payloadPtr->tank.tank.level);
				break;
			}
			case _packetID[PUMP].type:
			{
				Serial.print(F("               ID: "));
				Serial.println(_payloadPtr->pump.pump.id);
				Serial.print(F("            Power: "));
				// Don't know why I have to things this way, but I do...
			
	/*  	  	
				PH_powerSupply _powerVal;
				_powerVal = _packet.content.packetPayload.pump.pump.power;
				uint8_t _maskPowerVal = (uint8_t)_powerVal & 0xFF;
				PH_powerSupply _phMaskPowerVal = (PH_powerSupply)_maskPowerVal;
				if ( _phMaskPowerVal == PH_POWER_LIVE ) {
					Serial.println(F("LIVE"));
				} else if ( _phMaskPowerVal == PH_POWER_FAIL ) {
					Serial.println(F("FAIL"));
				} else {
					Serial.println(F("UNDEFINED"));
				}
	*/  	  
			
				PH_powerSupply _powerVal;
	//			Serial.println("[serialOut::powerSupply] Set return value...");
	//			Serial.print("[serialOut::powerSupply] Payload setting : 0x");
	//			Serial.println(_packet.content.packetPayload.pump.pump.power,HEX);
				_powerVal = _packet.content.packetPayload.pump.pump.power;
	//			Serial.print("[serialOut::powerSupply] Initialise _powerVal : 0x");
	//			Serial.println(_powerVal,HEX);
	//			Serial.print("[serialOut::pumpSupply] Mask byte : 0x");
				uint8_t _maskPowerVal = (uint8_t)_powerVal & 0xFF;
	//			Serial.println(_maskPowerVal,HEX);
	//			Serial.print("[serialOut::pumpSupply] Return value : 0x");
				PH_powerSupply _phMaskPowerVal = (PH_powerSupply)_maskPowerVal;
	//			Serial.println(_phMaskPowerVal,HEX);
				if ( _phMaskPowerVal == PH_POWER_LIVE ) {
					Serial.println(F("LIVE"));
				} else if ( _phMaskPowerVal == PH_POWER_FAIL ) {
					Serial.println(F("FAIL"));
				} else {
					Serial.println(F("UNDEFINED"));
				}
			
				Serial.print(F("            Relay: "));
				// Once again, don't know why I have to things this way, but I do...
				PH_relayState _relayVal;
	//			Serial.println("[serialOut::relayState] Set return value...");
	//			Serial.print("[serialOut::relayState] Payload setting : 0x");
	//			Serial.println(_packet.content.packetPayload.pump.pump.relay,HEX);
				_relayVal = _packet.content.packetPayload.pump.pump.relay;
	//			Serial.print("[serialOut::relayState] Initialise _relayVal : 0x");
	//			Serial.println(_relayVal,HEX);
	//			Serial.print("[serialOut::relayState] Mask byte : 0x");
				uint8_t _maskRelayVal = (uint8_t)_relayVal & 0xFF;
	//			Serial.println(_maskValVal,HEX);
	//			Serial.print("[serialOut::relayState] Return value : 0x");
				PH_relayState _phMaskRelayVal = (PH_relayState)_maskRelayVal;
	//			Serial.println(_phMaskRelayVal,HEX);
				if ( _phMaskRelayVal == PH_RELAY_ON ) {
					Serial.println(F("ON"));
				} else if ( _phMaskRelayVal == PH_RELAY_OFF ) {
					Serial.println(F("OFF"));
				} else {
					Serial.println(F("UNDEFINED"));
				}
				
				Serial.print(F("             Mode: "));
				// And again, don't know why I have to things this way, but I do...
				PH_controlMode _modeVal;
	//			Serial.println("[serialOut::controlMode] Set return value...");
	//			Serial.print("[serialOut::controlMode] Payload setting : 0x");
	//			Serial.println(_packet.content.packetPayload.pump.pump.mode,HEX);
				_modeVal = _packet.content.packetPayload.pump.pump.mode;
	//			Serial.print("[serialOut::controlMode] Initialise _modeVal : 0x");
	//			Serial.println(_modeVal,HEX);
	//			Serial.print("[serialOut::controlMode] Mask byte : 0x");
				uint8_t _maskModeVal = (uint8_t)_modeVal & 0xFF;
	//			Serial.println(_maskModeVal,HEX);
	//			Serial.print("[serialOut::controlMode] Return value : 0x");
				PH_controlMode _phMaskModeVal = (PH_controlMode)_maskModeVal;
	//			Serial.println(_phMaskModeVal,HEX);
				if ( _phMaskModeVal == PH_MODE_LOCAL ) {
					Serial.println(F("LOCAL"));
				} else if ( _phMaskModeVal == PH_MODE_REMOTE ) {
					Serial.println(F("REMOTE"));
				} else {
					Serial.println(F("UNDEFINED"));
				}

				break;
			}
			case _packetID[FLOW].type:
			{
				Serial.print(F("        Flow Rate: "));
				Serial.print((float)_payloadPtr->flow.recorded.rate/10, 1);
				Serial.println(F(" L/min"));
				Serial.print(F("      Flow Volume: "));
				Serial.print((float)_payloadPtr->flow.recorded.volume/1000, 1);
				Serial.println(F(" L"));
				break;
			}
			case _packetID[WEATHER].type:
			{
				Serial.print(F("      Temperature: "));
				Serial.print((float)_payloadPtr->weather.recorded.temperature/10, 1);
				Serial.println(F("°C"));
				Serial.print(F("         Pressure: "));
				Serial.print(_payloadPtr->weather.recorded.pressure);
				Serial.println(F(" hPa"));
				Serial.print(F("         Humidity: "));
				Serial.print(_payloadPtr->weather.recorded.humidity);
				Serial.println(F("%"));
				Serial.print(F("         Rainfall: "));
				Serial.print((float)_payloadPtr->weather.recorded.rainfall/10, 1);
				Serial.println(F(" mm"));
				Serial.print(F("   Wind Direction: "));
				Serial.print(_payloadPtr->weather.recorded.windBearing);
				Serial.println(F("°"));
				Serial.print(F("       Wind Speed: "));
				Serial.print(_payloadPtr->weather.recorded.windSpeed);
				Serial.println(F(" kph"));
				break;
			}
			case _packetID[ATMOSPHERE].type:
			{
				Serial.print(F("      Temperature: "));
				Serial.print((float)_payloadPtr->atmosphere.recorded.temperature/10, 1);
				Serial.println(F("°C"));
				Serial.print(F("         Pressure: "));
				Serial.print(_payloadPtr->atmosphere.recorded.pressure);
				Serial.println(F(" hPa"));
				Serial.print(F("         Humidity: "));
				Serial.print(_payloadPtr->atmosphere.recorded.humidity);
				Serial.println(F("%"));
				break;
			}
			case _packetID[TEMPERATURE].type:
			{
				Serial.print(F("      Temperature: "));
				Serial.print((float)_payloadPtr->temperature.recorded.temperature/10, 1);
				Serial.println(F("°C"));
				break;
			}
			case _packetID[RAINFALL].type:
			{
				Serial.print(F("         Rainfall: "));
				Serial.print((float)_payloadPtr->rainfall.recorded.rainfall/10, 1);
				Serial.println(F(" mm"));
				break;
			}
			case _packetID[WIND].type:
			{
				Serial.print(F("   Wind Direction: "));
				Serial.print(_payloadPtr->wind.recorded.windBearing);
				Serial.println(F("°"));
				Serial.print(F("       Wind Speed: "));
				Serial.print(_payloadPtr->wind.recorded.windSpeed);
				Serial.println(F(" kph"));
				break;
			}
			case _packetID[VOX].type:
			{
				Serial.print(F("            Level: "));
				Serial.print((float)_payloadPtr->vox.recorded.voxLevel/100, 2);
				Serial.println(F(" %"));
				break;
			}
			case _packetID[LIGHT].type:
			{
				Serial.print(F("          Ambient: "));
				Serial.print((float)_payloadPtr->light.recorded.ambientLight/100, 2);
				Serial.println(F(" lux"));
				break;
			}
			case _packetID[UV].type:
			{
				Serial.print(F("              UVA: "));
				Serial.print((float)_payloadPtr->uv.recorded.uvA/100, 2);
				Serial.println(F(" lux"));
				Serial.print(F("              UVB: "));
				Serial.print((float)_payloadPtr->uv.recorded.uvB/100, 2);
				Serial.println(F(" lux"));
				Serial.print(F("         UV Index: "));
				Serial.print((float)_payloadPtr->uv.recorded.uvIndex/100, 2);
				Serial.println(F(" lux"));
				break;
			}
			case _packetID[SPRINKLER].type:
			{
				// Nothing here yet
				Serial.println(F("[serialOut] Sprinkler details not currently recorded"));
				break;
			}
			case _packetID[GPS].type:
			{
				Serial.print(F("         Latitude: "));
				Serial.print((float)_payloadPtr->gps.position.latitude);
				Serial.println(F(" °"));
				Serial.print(F("        Longitude: "));
				Serial.print((float)_payloadPtr->gps.position.longitude);
				Serial.println(F(" °"));
				Serial.print(F("         Altitude: "));
				Serial.print((float)_payloadPtr->gps.position.altitude, 0);
				Serial.println(F(" m"));
				Serial.print(F("             HDOP: "));
				Serial.print((float)_payloadPtr->gps.position.hdop, 0);
				Serial.println(F(" "));
				break;
			}
			case _packetID[AWTS].type:
			{
				Serial.print(F("  Blower Pressure: "));
				Serial.print((float)_payloadPtr->awts.recorded.blowerPressure/100, 2);
				Serial.println(F(" kPa"));
				Serial.print(F("         IT Level: "));
				Serial.print(_payloadPtr->awts.recorded.itLevel);
				Serial.println(F("%"));
				break;
			}
			case _packetID[RESET].type:
			{
				Serial.print(F("       Error Code: "));
				Serial.println(_payloadPtr->reset.error.code);
				break;
			}
			default:
			{
				Serial.println();
				Serial.print(F("[serialOut] Unknown Packet Type : [Hex] "));
				Serial.print(_packet.content.packetType,HEX);
				Serial.println();
			}
		}
	}
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void PacketHandler::displayOut(SSD1306 *_displayPtr) {
#elif defined(__ASR_Arduino__)
void PacketHandler::displayOut(SH1107Wire *_displayPtr) {
#endif
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32) || defined(__ASR_Arduino__)
//  Serial.println("[displayOut] Update display...");

// Get the index to the incoming packet source Node details

//  NodeHandler _node;
  int _nodeIndex = _node.index(_packet.content.sourceMAC);

// Get a pointer [that understands the structure] to the payload

  PH_genericPayload *_payloadPtr;
  _payloadPtr = &_packet.content.packetPayload;

// Initialise the display and get on with processing the packet

  _displayPtr->clear();
  _displayPtr->setFont(ArialMT_Plain_10);
  _displayPtr->setTextAlignment(TEXT_ALIGN_CENTER);
  _displayPtr->drawString(72, 0, String(_nodeIndex));
  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  _displayPtr->drawString(128, 0, String(_packet.content.sequenceNumber));

  switch (_packet.content.packetType & _typeMask)
  {
    case _packetID[ACK].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "ACK");
  	  break;
    }
    case _packetID[VOLTAGE].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Voltage");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Battery");
      _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float)_payloadPtr->voltage.source.voltage/1000,2) + " V");
  	  break;
    }
    case _packetID[POWER].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Power");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(56, 12, "Battery");
  	  _displayPtr->drawString(56, 24, "Panel");
  	  _displayPtr->drawString(56, 36, "Load");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(60, 12, String((float)_payloadPtr->power.battery.voltage,2) + "V");
  	  _displayPtr->drawString(60, 24, String((float)_payloadPtr->power.panel.voltage,2) + "V");
  	  _displayPtr->drawString(60, 36, String((float)_payloadPtr->power.load.voltage,2) + "V");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(128, 12, String(_payloadPtr->power.battery.current) + "mA");
  	  _displayPtr->drawString(128, 24, String(_payloadPtr->power.panel.current) + "mA");
  	  _displayPtr->drawString(128, 36, String(_payloadPtr->power.load.current) + "mA");
  	  break;
    }
    case _packetID[TANK].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Tank");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "ID");
  	  _displayPtr->drawString(60, 24, "Level");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->tank.tank.id));        
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->tank.tank.level));        
  	  break;
    }
    case _packetID[PUMP].type:
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Pump");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "ID");
  	  _displayPtr->drawString(60, 20, "Status");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->pump.pump.id));        
// I don't think this is right... it reamins really just as a place holder...
  	  _displayPtr->drawString(64, 20, String(_payloadPtr->pump.pump.power));        
// Same here...
  	  _displayPtr->drawString(64, 28, String(_payloadPtr->pump.pump.relay));        
// Same here...
  	  _displayPtr->drawString(64, 36, String(_payloadPtr->pump.pump.mode));        
  	  break;
    case _packetID[FLOW].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Flow");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Rate");
  	  _displayPtr->drawString(60, 24, "Volume");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float) _payloadPtr->flow.recorded.rate/10,1) + " L/min");        
  	  _displayPtr->drawString(64, 24, String((float)_payloadPtr->flow.recorded.volume/1000,1) + " L");        
  	  break;
    }
    case _packetID[WEATHER].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Weather");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Temperature");
  	  _displayPtr->drawString(60, 24, "Pressure");
  	  _displayPtr->drawString(60, 36, "Humidity");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float) _payloadPtr->weather.recorded.temperature/10,1) + "°C");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->weather.recorded.pressure) + " hPa");
  	  _displayPtr->drawString(64, 36, String(_payloadPtr->weather.recorded.humidity) + "%");
	  // There's not enough room on the display for any more
  	  break;
    }
    case _packetID[ATMOSPHERE].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Atmosphere");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Temperature");
  	  _displayPtr->drawString(60, 24, "Pressure");
  	  _displayPtr->drawString(60, 36, "Humidity");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float) _payloadPtr->atmosphere.recorded.temperature/10,1) + "°C");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->atmosphere.recorded.pressure) + " hPa");
  	  _displayPtr->drawString(64, 36, String(_payloadPtr->atmosphere.recorded.humidity) + "%");
  	  break;
    }
    case _packetID[TEMPERATURE].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Temperature");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Temperature");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float) _payloadPtr->temperature.recorded.temperature/10,1) + "°C");
  	  break;
    }
    case _packetID[RAINFALL].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Rainfall");
      _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
      _displayPtr->drawString(60, 12, "Rainfall");
      _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
      _displayPtr->drawString(64, 12, String((float) _payloadPtr->rainfall.recorded.rainfall/10,1) + " mm");
      break;
    }
    case _packetID[WIND].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Wind");
  	  
      _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
      _displayPtr->drawString(60, 12, "Direction");
  	  _displayPtr->drawString(60, 24, "Speed");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->wind.recorded.windBearing) + "°");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->wind.recorded.windSpeed) + " kph");
  	  break;
    }
    case _packetID[VOX].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "VOX");
  	  
      _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
      _displayPtr->drawString(60, 12, "Level");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float) _payloadPtr->vox.recorded.voxLevel/100,2) + "%");
  	  break;
    }
    case _packetID[LIGHT].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Ambient Light");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Light");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->light.recorded.ambientLight/100,2) + " lux");
  	  break;
    }
    case _packetID[UV].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "UV");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "UVA");
  	  _displayPtr->drawString(60, 24, "UVB");
  	  _displayPtr->drawString(60, 36, "UV Index");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->uv.recorded.uvA/100,2) + " lux");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->uv.recorded.uvB/100,2) + " lux");
  	  _displayPtr->drawString(64, 36, String(_payloadPtr->uv.recorded.uvIndex/100,2) + " lux");
  	  break;
    }
    case _packetID[SPRINKLER].type:
//      Nothing here yet
  	break;
    case _packetID[GPS].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "GPS");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Latitude");
  	  _displayPtr->drawString(60, 24, "Longitude");
  	  _displayPtr->drawString(60, 36, "Altitude");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String(_payloadPtr->gps.position.latitude,2) + "°");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->gps.position.longitude,2) + "°");
  	  _displayPtr->drawString(64, 36, String(_payloadPtr->gps.position.altitude) + " m");
  	  break;
    }
    case _packetID[AWTS].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "AWTS");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Pressure");
  	  _displayPtr->drawString(60, 24, "Level");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64, 12, String((float)_payloadPtr->awts.recorded.blowerPressure/100,2) + " kPa");
  	  _displayPtr->drawString(64, 24, String(_payloadPtr->awts.recorded.itLevel) + "%");
  	  break;
    }
    case _packetID[RESET].type:
    {
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(0, 0, "Reset");
  	  
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_RIGHT);
  	  _displayPtr->drawString(60, 12, "Error Code");
  	  _displayPtr->setTextAlignment(TEXT_ALIGN_LEFT);
  	  _displayPtr->drawString(64,12, String(_payloadPtr->reset.error.code));        
  	  break;
    }
//    default:
//      Nothing here yet
  }
  _displayPtr->display();
}
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void PacketHandler::mqttOut(PubSubClient *_clientPtr) {

  // Assumes MQTT session already in place

  char _topicBuffer[PH_topicBufferSize];
  String _localString;

// Get the index of the incoming packet Node details

  int _nodeIndex = _node.index(_packet.content.sourceMAC);

// Get a pointer [that understands the structure] to the payload

  PH_genericPayload *_payloadPtr;
  _payloadPtr = &_packet.content.packetPayload;

// First, publish the heartbeat for regular packets (not ACK packets)

	if ( !this->ackFlag() ) {
//  	Serial.println("[mqttOut] ACK Flag not set, sending Heartbeat...");
		char _sequenceNumberChar[5];
		itoa(_packet.content.sequenceNumber, _sequenceNumberChar, 10);

		_localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_HEARTBEAT];
		_localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
		_clientPtr->publish(_topicBuffer, _sequenceNumberChar);

//  	Serial.println("[mqttOut] Message sent : " + _localString);
	}

// and reset the Node's heartbeat timer

  _node.resetTimer(_nodeIndex);

// Now publish topics according to Node ID and packet type

/*
 * Note that the char arrays used to stringify numerical values (string or float)
 * must be sized to handle:
 *  1.	The largest possible number to be converted
 *			For a 16 bit unsigned integer, the range is 0..65531
 *			For a 16 bit signed integer, the rangeis is -32768?..32767
 *	2.	Any possible "." (decimal point) or "-" (minus sign)
 *	3.	The null-terminating character "\0"
 
 *	Thus, for a 16 bit signed integer, the minimum size for the char array should be 7:
 *			5 char for the largest number
 *			1 char for the possible minus sign
 *			1 char for the null terminator
 *
 *	If the number is to be expressed with a decimal place (as a floating point number),
 *  then additional chars may be required for the decimal place itself and any digits of
 *  precision that are to be included.
 *
 *	Format for dtostrf() is:
 *			dtostrf(float_value, min_width, num_digits_after_decimal, receiving_char_array)
 */

  switch (_packet.content.packetType & _typeMask) {
    case _packetID[ACK].type:
    {
// See PUMP

      _localString = "[mqttOut] ACK MQTT message yet to be defined...";
//      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[VOLTAGE].type:
    {
      char _voltageChar[7];
      dtostrf((float)_payloadPtr->voltage.source.voltage/1000, 4, 2, _voltageChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_VOLTAGE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _voltageChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[POWER].type:
    {
      char _voltageChar[7];
      dtostrf((float)_payloadPtr->power.battery.voltage/1000, 4, 2, _voltageChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_BATTERY_VOLTAGE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _voltageChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      dtostrf((float)_payloadPtr->power.panel.voltage/1000, 4, 2, _voltageChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PANEL_VOLTAGE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _voltageChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      dtostrf((float)_payloadPtr->power.load.voltage/1000, 4, 2, _voltageChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_LOAD_VOLTAGE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _voltageChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _currentChar[8];
      dtostrf(_payloadPtr->power.battery.current, 7, 1, _currentChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_BATTERY_CURRENT];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _currentChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      dtostrf(_payloadPtr->power.panel.current, 7, 1, _currentChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PANEL_CURRENT];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _currentChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      dtostrf(_payloadPtr->power.load.current, 7, 1, _currentChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_LOAD_CURRENT];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _currentChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[TANK].type:
    {
      char _idChar[3];
      itoa(_payloadPtr->tank.tank.id, _idChar, 16);
      char _levelChar[7];
      itoa(_payloadPtr->tank.tank.level, _levelChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_TANK_LEVEL] + "/" + _idChar;
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _levelChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[PUMP].type:
    {
      char _idChar[3];
      itoa(_payloadPtr->pump.pump.id, _idChar, 16);
      char _onChar[10];
			String _stateString;
      char _modeChar[10];
			String _modeString;

/*
			PH_pumpState _val;
			_val = (PH_pumpState)((bool)_payloadPtr->pump.power.on & 0xFF);
  	  if ( _val == PH_POWER_ON ) {
        _stateString = "ON";
  	  } else if ( _val == PH_POWER_OFF ) {
        _stateString = "OFF";
  	  } else {
				_stateString = "Undefined";
  	  }
*/  	  

  	  PH_powerSupply _powerVal;
//			Serial.println("[mqttOut::powerSupply] Set return value...");
//			Serial.print("[mqttOut::powerSupply] Payload setting : 0x");
//			Serial.println(_packet.content.packetPayload.pump.pump.power,HEX);
			_powerVal = _packet.content.packetPayload.pump.pump.power;
//			Serial.print("[mqttOut::powerSupply] Initialise _power : 0x");
//			Serial.println(_powerVal,HEX);
//			Serial.print("[mqttOut::powerSupply] Mask byte : 0x");
			uint8_t _maskPowerVal = (uint8_t)_powerVal & 0xFF;
//			Serial.println(_maskPowerVal,HEX);
//			Serial.print("[mqttOut::powerSupply] Return value : 0x");
			PH_powerSupply _phMaskPowerVal = (PH_powerSupply)_maskPowerVal;
//			Serial.println(_phMaskPowerVal,HEX);
  	  if ( _phMaskPowerVal == PH_POWER_LIVE ) {
        _stateString = "LIVE";
  	  } else if ( _phMaskPowerVal == PH_POWER_FAIL ) {
        _stateString = "FAIL";
  	  } else {
				_stateString = "UNDEFINED";
  	  }
//	  	Serial.println("[mqttOut] _stateString : " + _stateString);
	  
			_stateString.toCharArray(_onChar, 10);
			
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PUMP_POWER] + "/" + _idChar;
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _onChar);      
      _localString = "[mqttOut] " + _localString + " " + _stateString + " message sent";
      Serial.println(_localString);

  	  PH_relayState _relayVal;
			_relayVal = _packet.content.packetPayload.pump.pump.relay;
			uint8_t _maskRelayVal = (uint8_t)_relayVal & 0xFF;
			PH_relayState _phMaskRelayVal = (PH_relayState)_maskRelayVal;
  	  if ( _phMaskRelayVal == PH_RELAY_ON ) {
        _stateString = "ON";
  	  } else if ( _phMaskRelayVal == PH_RELAY_OFF ) {
        _stateString = "OFF";
  	  } else {
				_stateString = "UNDEFINED";
  	  }
//	  	Serial.println("[mqttOut] _stateString : " + _stateString);
	  
			_stateString.toCharArray(_onChar, 10);
			
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PUMP_RELAY] + "/" + _idChar;
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _onChar);      
      _localString = "[mqttOut] " + _localString + " " + _stateString + " message sent";
      Serial.println(_localString);
      
  	  PH_controlMode _modeVal;
			_modeVal = _packet.content.packetPayload.pump.pump.mode;
			uint8_t _maskModeVal = (uint8_t)_modeVal & 0xFF;
			PH_controlMode _phMaskModeVal = (PH_controlMode)_maskModeVal;
  	  if ( _phMaskModeVal == PH_MODE_LOCAL ) {
        _modeString = "LOCAL";
  	  } else if ( _phMaskModeVal == PH_MODE_REMOTE ) {
        _modeString = "REMOTE";
  	  } else {
				_modeString = "UNDEFINED";
  	  }
//	  	Serial.println("[mqttOut] _modeString : " + _modeString);
	  	
			_modeString.toCharArray(_modeChar, 10);
			
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PUMP_MODE] + "/" + _idChar;
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _modeChar);      
      _localString = "[mqttOut] " + _localString + " " + _modeString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[FLOW].type:
    {
      char _rateChar[7];
      dtostrf((float)_payloadPtr->flow.recorded.rate/10, 3, 1, _rateChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_FLOW_RATE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _rateChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _volumeChar[10];
      dtostrf((float)_payloadPtr->flow.recorded.volume/1000, 3, 1, _volumeChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_FLOW_VOLUME];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _volumeChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[WEATHER].type:
    {
      char _temperatureChar[7];
      dtostrf((float)_payloadPtr->weather.recorded.temperature/10, 3, 1, _temperatureChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_TEMPERATURE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _temperatureChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _humidityChar[7];
      dtostrf(_payloadPtr->weather.recorded.humidity, 5, 1, _humidityChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_HUMIDITY];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _humidityChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
 
      char _pressureChar[7];
      dtostrf(_payloadPtr->weather.recorded.pressure, 5, 1, _pressureChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PRESSURE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _pressureChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _rainfallChar[7];
      dtostrf((float)_payloadPtr->weather.recorded.rainfall/10, 3, 1, _rainfallChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_RAINFALL];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _rainfallChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
 
      char _windBearingChar[5];
      itoa(_payloadPtr->weather.recorded.windBearing, _windBearingChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_WIND_BEARING];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _windBearingChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _windSpeedChar[5];
      itoa(_payloadPtr->weather.recorded.windSpeed, _windSpeedChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_WIND_SPEED];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _windSpeedChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      break;
    }
    case _packetID[ATMOSPHERE].type:
    {
      char _temperatureChar[7];
      dtostrf((float)_payloadPtr->atmosphere.recorded.temperature/10, 3, 1, _temperatureChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_TEMPERATURE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _temperatureChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _humidityChar[7];
      dtostrf(_payloadPtr->atmosphere.recorded.humidity, 3, 1, _humidityChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_HUMIDITY];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _humidityChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
 
      char _pressureChar[7];
      dtostrf(_payloadPtr->atmosphere.recorded.pressure, 4, 1, _pressureChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_PRESSURE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _pressureChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[TEMPERATURE].type:
    {
      char _temperatureChar[7];
      
      dtostrf((float)_payloadPtr->temperature.recorded.temperature/10, 3, 1, _temperatureChar);
			_localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_TEMPERATURE];
			_localString.toCharArray(_topicBuffer, PH_topicBufferSize);
			_clientPtr->publish(_topicBuffer, _temperatureChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[RAINFALL].type:
    {
      char _rainfallChar[7];
      dtostrf((float)_payloadPtr->rainfall.recorded.rainfall/10, 3, 1, _rainfallChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_RAINFALL];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _rainfallChar);
            _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[WIND].type:
    {
      char _windBearingChar[5];
      itoa(_payloadPtr->wind.recorded.windBearing, _windBearingChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_WIND_BEARING];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _windBearingChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _windSpeedChar[5];
      itoa(_payloadPtr->wind.recorded.windSpeed, _windSpeedChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_WIND_SPEED];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _windSpeedChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[VOX].type:
    {
      char _voxChar[7];
      dtostrf((float)_payloadPtr->vox.recorded.voxLevel/100, 3, 2, _voxChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_VOX];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _voxChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[LIGHT].type:
    {
      char _lightChar[7];
      dtostrf((float)_payloadPtr->light.recorded.ambientLight/100, 3, 2, _lightChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_AMBIENT_LIGHT];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _lightChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[UV].type:
    {
      char _uvAChar[7];
      dtostrf((float)_payloadPtr->uv.recorded.uvA/100, 3, 2, _uvAChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_UV_A];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _uvAChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _uvBChar[7];
      dtostrf((float)_payloadPtr->uv.recorded.uvB/100, 3, 2, _uvBChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_UV_B];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _uvBChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _uvIndexChar[7];
      dtostrf((float)_payloadPtr->uv.recorded.uvIndex/100, 3, 2, _uvIndexChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_UV_INDEX];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _uvIndexChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
  	  break;
    }
    case _packetID[SPRINKLER].type:
    {
      Serial.println("[mqttOut] Sprinkler details not currently recorded");
      break;
    }
    case _packetID[GPS].type:
    {
      char _latitudeChar[16];
      dtostrf(_payloadPtr->gps.position.latitude, 6, 2, _latitudeChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_LATITUDE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _latitudeChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _longitudeChar[16];
      dtostrf(_payloadPtr->gps.position.longitude, 6, 2, _longitudeChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_LONGITUDE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _longitudeChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      char _altitudeChar[8];
      dtostrf(_payloadPtr->gps.position.altitude, 4, 2, _altitudeChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_ALTITUDE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _altitudeChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[AWTS].type:
    {
      char _blowerPressureChar[7];
      dtostrf((float)_payloadPtr->awts.recorded.blowerPressure/100, 2, 2, _blowerPressureChar);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_AWTS_PRESSURE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _blowerPressureChar);
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);

      char _itLevelChar[5];
      itoa(_payloadPtr->awts.recorded.itLevel, _itLevelChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_AWTS_LEVEL];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);        
      _clientPtr->publish(_topicBuffer, _itLevelChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
    case _packetID[RESET].type:
    {
      char _codeChar[7];
      itoa(_payloadPtr->reset.error.code, _codeChar, 10);
      _localString = _node.topic(_nodeIndex) + "/" + SENSOR_TOPIC[T_RESET_CODE];
      _localString.toCharArray(_topicBuffer, PH_topicBufferSize);
      _clientPtr->publish(_topicBuffer, _codeChar);      
      _localString = "[mqttOut] " + _localString + " message sent";
      Serial.println(_localString);
      
      break;
    }
//    default:
//      Nothing here yet
  }
}
#endif