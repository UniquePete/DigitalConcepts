/*
    EepromHandler - Langlo EEPROM management
    Version 0.0.16	22/05/2025

    0.0.1 Initial release
    0.0.2 Change setAddressingMode() to initSmartSerial()
    0.0.3 Add getI2CAddress() method
          Expand EH_dataType to include MAC addresses and descriptor
          Add methods to get EH_dataType byte counts and locations
    0.0.4 Add dump()and scrub() methods
    0.0.5 Move softwareRevision() to Public
		0.0.6 Change const to static const/constexpr as required for LinkedList compatibility
		0.0.7 Change _byteCount and _location to uint8_t for compatibility with Wire.h library
		0.0.8 Merge _byteCount and _location into singe struct _element
		0.0.9 Increase the size of the Descriptor element from 16 to 48 bytes
		0.0.10 Correct the specification of the Descriptor element size
		0.0.11 Amend dump()/scrub() functions to avoid problems with large reads and writes
		0.0.12 Delete EH_MAX_BYTE_COUNT
		0.0.13 Change initSmartSerial() to setSmartSerial() (no parameters)
		       Remove SmartSerial initialisation from init() & begin()
		       Set SmartSerial to true by default
		0.0.14 Add PumpID and TankID to the list of stored values
		0.0.15 Amend read() to return 0 on fail
		0.0.16 Add bool isConnected() function to verify EEPROM accessibility
		
    Digital Concepts
    22 May 2025
    digitalconcepts.net.au
   
    This is just a convenient way to keep all the information relating to EEPROM data storage
    in one place.
   
    The external EEPROM included on many of the PCBs is used, in spite of the fact that some
    processors include their own [versions of an] EEPROM, to store a sequence number, for
    packet sequencing, and counters currently used to record rainfall in the Weather
    Station application.
*/

#include "EepromHandler.h"

constexpr EepromHandler::EH_element EepromHandler::_element[];
    
#ifndef SoftwareRevision_def
  #define SoftwareRevision_def
  struct _SoftwareRevision {
		const uint8_t major;			// Major feature release
		const uint8_t minor;			// Minor feature enhancement release
		const uint8_t minimus;		// 'Bug fix' release
  };
#endif

_SoftwareRevision _eepromHandlerRevNumber = {0,0,16};

EepromHandler::EepromHandler() {
}
   
String EepromHandler::softwareRevision() {
	String _softwareRev;
	_softwareRev = String(_eepromHandlerRevNumber.major) + "." + String(_eepromHandlerRevNumber.minor) + "." + String(_eepromHandlerRevNumber.minimus);
	return _softwareRev;
}

bool EepromHandler::isConnected() {
  _i2cBusPtr->beginTransmission(_i2cAddress);
  if (_i2cBusPtr->endTransmission() == 0)
    return (true);
  return (false);
}

void EepromHandler::begin(TwoWire* _wirePtr) {
  init(_wirePtr, _defaultI2cAddress);
  this->_smartSerial = true;
}

void EepromHandler::begin(TwoWire* _wirePtr, uint8_t _address) {
  init(_wirePtr, _address);
  this->_smartSerial = true;
}

void EepromHandler::begin(TwoWire* _wirePtr, bool _modeFlag) {
  init(_wirePtr, _defaultI2cAddress);
  this->_smartSerial = _modeFlag;
}

void EepromHandler::begin(TwoWire* _wirePtr, uint8_t _address, bool _modeFlag) {
  init(_wirePtr, _address);
  this->_smartSerial = _modeFlag;
}

void EepromHandler::init(TwoWire* _wirePtr, uint8_t _address) {
  this->_i2cBusPtr = _wirePtr;
  this->_i2cAddress = _address;
}

void EepromHandler::setI2CBus(TwoWire* _wire) {
  this->_i2cBusPtr = _wire;
}

void EepromHandler::setI2CAddress(uint8_t _address) {
  this->_i2cAddress = _address;
}

uint8_t EepromHandler::getI2CAddress() {
  return this->_i2cAddress;
}

bool EepromHandler::setSmartSerial() {
  uint8_t _stdBuf[2], _smtBuf[2], _readTestBuf[2], _writeTestBuf[2];
  
  _readTestBuf[0] = 0x0;
  _writeTestBuf[0] = 0x1;
  _writeTestBuf[1] = 0x1;
  
// Save the EEPROM content of the area we're about to use
// Use both addressing methods to retrieve the contents. When we know which addressing
// method is the correct one, we'll use that to restore the value that was retrieved using
// that method.

  if ( _EH_DEBUG ) { 
    Serial.println(F("[EepromHandler::setSmartSerial] Saving current value..."));
    Serial.println(F("[EepromHandler::setSmartSerial] Using Standard Addressing..."));
  }
  _smartSerial = false;
  this->read(_stdBuf,2,0);

  if ( _EH_DEBUG ) { Serial.println(F("[EepromHandler::setSmartSerial] Using Smart Addressing...")); }
  _smartSerial = true;
  this->read(_smtBuf,2,0);

// Now determine whether or not we need to use smart addressing
// First, write using Smart Addressing
  
//_EH_DEBUG = true;  

  if ( _EH_DEBUG ) { Serial.println(F("[EepromHandler::setSmartSerial] Writing test value...")); }
  this->write(_writeTestBuf,1,1);
  this->write(_writeTestBuf,1,0);
  
// Next, read using Standard addressing

  _smartSerial = false;
  if ( _EH_DEBUG ) { Serial.println(F("[EepromHandler::setSmartSerial] Standard Reading test value...")); }
  this->read(_readTestBuf,1,0);
  
//_EH_DEBUG = false;  
  
// If EEPROM required Standard Addressing, we will read what was just written to location 0000, i.e. "0".
// If not, Smart Addressing must be required.

  if ( _readTestBuf[0] == 0x0 ) {
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::setSmartSerial] EEPROM: 16K-, Standard Addressing"));
      Serial.println(F("[EepromHandler::setSmartSerial] Restoring saved value..."));
    }
    _smartSerial = false;
    this->write(_stdBuf,2,0);
  } else {
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::setSmartSerial] EEPROM: 32K+, Smart Addressing"));
      Serial.println(F("[EepromHandler::setSmartSerial] Restoring saved value..."));
    }
    _smartSerial = true;
    this->write(_smtBuf,2,0);
  }
  if ( _EH_DEBUG ) { Serial.println(F("[EepromHandler::getSmartSerial] Check complete")); }
  
  return _smartSerial;
}

void EepromHandler::setSmartSerial(bool _value) {
  if ( _EH_DEBUG ) { 
    if ( _value ) {
      Serial.println(F("[EepromHandler::setSmartSerial] _value : On"));
    } else { 
      Serial.println(F("[EepromHandler::setSmartSerial] _value : Off"));
    }
  }
  this->_smartSerial = _value;
  if ( _EH_DEBUG ) { 
    if ( this->_smartSerial ) {
      Serial.println(F("[EepromHandler::setSmartSerial] this->_smartSerial : On"));
    } else { 
      Serial.println(F("[EepromHandler::setSmartSerial] this->_smartSerial : Off"));
    }
  }
}

bool EepromHandler::getSmartSerial() {
  if ( _EH_DEBUG ) { 
    if ( this->_smartSerial ) {
      Serial.println(F("[EepromHandler::setSmartSerial] this->_smartSerial : On"));
    } else { 
      Serial.println(F("[EepromHandler::setSmartSerial] this->_smartSerial : Off"));
    }
  }
  return this->_smartSerial;
}

void EepromHandler::setDebug(bool _debug) {
  this->_EH_DEBUG = _debug;
}

bool EepromHandler::getDebug() {
  if ( _EH_DEBUG ) {
    Serial.println(F("[EepromHandler::setDebug] Debug : On"));
  } else{ 
    Serial.println(F("[EepromHandler::setDebug] Debug : Off"));
  }
  return _EH_DEBUG;
}

uint8_t EepromHandler::getParameterByteCount(EH_dataType _dataType) {
  return _element[_dataType].byteCount;
}

uint8_t EepromHandler::getParameterLocation(EH_dataType _dataType) {
  return _element[_dataType].location;
}

void EepromHandler::write(uint8_t* _buf, uint8_t _byteCount, uint8_t _eepromAddress) {
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::write] Writing EEPROM at location: "));
    Serial.println(_eepromAddress);
    Serial.print(F("[EepromHandler::write] Bytes to write : "));
    Serial.println(_byteCount);
  }
  _i2cBusPtr->beginTransmission(_i2cAddress);
  if (_smartSerial) {
    _i2cBusPtr->write(_eepromAddress >> 8);   // MSB
    _i2cBusPtr->write(_eepromAddress & 0xFF); delay(10);  // LSB
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::write] smartSerial : true"));
      Serial.print(F("[EepromHandler::write] Address HByte : 0x"));
      Serial.println(_eepromAddress >> 8,HEX);
      Serial.print(F("[EepromHandler::write] Address LByte : 0x"));
      Serial.println(_eepromAddress & 0xFF,HEX);
    }
  } else {
    _i2cBusPtr->write(_eepromAddress); delay(10);
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::write] smartSerial : false"));
      Serial.print(F("[EepromHandler::write] Address Simple : 0x"));
      Serial.println(_eepromAddress,HEX);
    }
  }

  for (int _x = 0 ; _x < _byteCount ; _x++) {
    _i2cBusPtr->write(_buf[_x]);                      // Write the data
    if ( _EH_DEBUG ) {
      Serial.print(F("[EepromHandler::write] Data Byte ") );
      Serial.print( _x );
      Serial.print(F(" : 0x") );
      Serial.println( _buf[_x],HEX );
    }
  }
  _i2cBusPtr->endTransmission(); delay(10);
}

void EepromHandler::writeUint32(EH_dataType _dataType, uint32_t _value) {
  _eepromData.macAddress = _value;
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeUint32] Parameter : 0x"));
    Serial.print(_dataType,HEX);
    switch (_dataType) {
      case EH_GATEWAY_MAC: {
        Serial.println(F(" (EH_GATEWAY_MAC)"));
        break;
      }
      case EH_NODE_MAC: {
        Serial.println(F(" (EH_NODE_MAC)"));
        break;
      }
      default: {
        Serial.println(F(" (Data Type Unrecognised or not uint32_t)"));
        break;
      }
    }
    Serial.print(F("[EepromHandler::writeData] Value : 0x"));
    Serial.println(_value,HEX);
  }
  this->write(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
}

void EepromHandler::writeUint16(EH_dataType _dataType, uint16_t _value) {
  _eepromData.counter = _value;
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeUint16] Parameter : 0x"));
    Serial.print(_dataType,HEX);
    switch (_dataType) {
      case EH_SEQUENCE: {
        Serial.println(F(" (EH_SEQUENCE)"));
        break;
      }
      case EH_RAINFALL: {
        Serial.println(F(" (EH_RAINFALL)"));
        break;
      }
      default: {
        Serial.println(F(" (Data Type Unrecognised or not uint16_t)"));
        break;
      }
    }
    Serial.print(F("[EepromHandler::writeData] Value : 0x"));
    Serial.println(_value,HEX);
  }
  this->write(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
}

void EepromHandler::writeUint8(EH_dataType _dataType, uint8_t _value) {
  _eepromData.counter = _value;
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeUint8] Parameter : 0x"));
    Serial.print(_dataType,HEX);
    switch (_dataType) {
      case EH_TANKID: {
        Serial.println(F(" (EH_TANKID)"));
        break;
      }
      case EH_PUMPID: {
        Serial.println(F(" (EH_PUMPID)"));
        break;
      }
      default: {
        Serial.println(F(" (Data Type Unrecognised or not uint8_t)"));
        break;
      }
    }
    Serial.print(F("[EepromHandler::writeData] Value : 0x"));
    Serial.println(_value,HEX);
  }
  this->write(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
}

void EepromHandler::writeBytes(EH_dataType _dataType, uint8_t* _buf) {
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeBytes] Parameter : 0x"));
    Serial.print(_dataType,HEX);
    switch (_dataType) {
      case EH_DESCRIPTOR: {
        Serial.println(F(" (EH_DESCRIPTOR)"));
        break;
      }
      default: {
        Serial.println(F(" (Data Type Unrecognised or not uint8_t*)"));
        break;
      }
    }
    Serial.print(F("[EepromHandler::writeBytes] Value : 0x"));
    for (int i = 0; i < _element[_dataType].byteCount; i++) {
      if (_buf[i] < 0x10) Serial.print("0");
      Serial.print(_buf[i],HEX);
      if ((i % 4) == 3) {
        Serial.println();
        if (i < (_element[_dataType].byteCount-1)) Serial.print(F("                   "));
      } else {
        Serial.print("  ");
      }
    }
    if ((_element[_dataType].byteCount % 4) != 0) Serial.println();
  }
  this->write(_buf, _element[_dataType].byteCount, _element[_dataType].location);
}

void EepromHandler::writeBytes(EH_dataType _dataType, uint8_t* _buf, uint8_t _numBytes) {
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeBytes] Parameter : 0x"));
    Serial.print(_dataType,HEX);
    switch (_dataType) {
      case EH_DESCRIPTOR: {
        Serial.println(F(" (EH_DESCRIPTOR)"));
        break;
      }
      default: {
        Serial.println(F(" (Data Type Unrecognised or not uint8_t*)"));
        break;
      }
    }
    
    Serial.print(F("[EepromHandler::writeBytes] Bytes to write : "));
    Serial.println(_numBytes);
  }
  if (_numBytes > _element[_dataType].byteCount) {
    _numBytes = _element[_dataType].byteCount;
    Serial.println(F("[EepromHandler::writeBytes] WARNING : Request exceeds capacity"));
    Serial.print(F("[EepromHandler::writeBytes] Bytes to write trimmed to : "));
    Serial.println(_numBytes);
  }
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::writeBytes] Value : 0x"));
    for (int i = 0; i < _numBytes; i++) {
      if (_buf[i] < 0x10) Serial.print("0");
      Serial.print(_buf[i],HEX);
      if ((i % 4) == 3) {
        Serial.println();
        if (i < (_numBytes-1)) Serial.print(F("                                     "));
      } else {
        Serial.print("  ");
      }
    }
    if ((_numBytes % 4) != 0) Serial.println();
  }
  uint8_t* _bufPtr;
  int _bytesToWrite = _numBytes;
  int _loc = _element[_dataType].location;
  for (int j = 0; j < _numBytes; j = j + 16) {
    _bufPtr = &_buf[j];
    if (_bytesToWrite > 16) {
      this->write(_bufPtr, 16, _loc);
      _bytesToWrite = _bytesToWrite - 16;
      _loc = _loc + 16;
    } else if (_bytesToWrite > 0){
      this->write(_bufPtr, _bytesToWrite, _loc);
    }
  }
}

void EepromHandler::read(uint8_t* _buf, uint8_t _byteCount, uint8_t _eepromAddress) {
  _i2cBusPtr->beginTransmission(_i2cAddress);
  if (_smartSerial) {
    _i2cBusPtr->write(_eepromAddress >> 8); // MSB
    _i2cBusPtr->write(_eepromAddress & 0xFF); delay(10);// LSB
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::read] Smart Serial (Two Byte) Addressing"));
      Serial.print(F("[EepromHandler::read] Address HByte : 0x"));
      Serial.println(_eepromAddress >> 8,HEX);
      Serial.print(F("[EepromHandler::read] Address LByte : 0x"));
      Serial.println(_eepromAddress & 0xFF,HEX);
    }
   } else {
    _i2cBusPtr->write( _eepromAddress ); delay(10); 
    if ( _EH_DEBUG ) {
      Serial.println(F("[EepromHandler::read] Standard (Single Byte) Addressing"));
      Serial.print(F("[EepromHandler::read] Address Simple : 0x"));
      Serial.println(_eepromAddress,HEX);
    }
  }
  if ( _EH_DEBUG ) {
    Serial.println(F("[EepromHandler::read] Ending address transmission..."));
  }
  _i2cBusPtr->endTransmission(); delay(10);
  if ( _EH_DEBUG ) {
    Serial.println(F("[EepromHandler::read] Requesting data..."));
  }
  _i2cBusPtr->requestFrom(_i2cAddress, _byteCount); delay(10);
  
  if ( _EH_DEBUG ) {
    Serial.println(F("[EepromHandler::read] Transferring data..."));
  }
  uint8_t _rdata = 0;
  for (int _x = 0 ; _x < _byteCount ; _x++) {
    if (_i2cBusPtr->available()) {
      _rdata = _i2cBusPtr->read();
      if ( _EH_DEBUG ) {
        Serial.print(F("[EepromHandler::read] Data Byte ") );
        Serial.print( _x );
        Serial.print( F(" : 0x") );
        Serial.println( _rdata,HEX );
      }
    } else {
      if ( _EH_DEBUG ) { Serial.println(F("[EepromHandler::read] Nothing to read...")); }
    }
		_buf[_x] = _rdata;
  }
}

uint32_t EepromHandler::readUint32(EH_dataType _dataType) {
  this->read(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
  return _eepromData.macAddress;
}

uint16_t EepromHandler::readUint16(EH_dataType _dataType) {
  this->read(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
  return _eepromData.counter;
}

uint8_t EepromHandler::readUint8(EH_dataType _dataType) {
  this->read(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
  return _eepromData.macAddress;
}

uint8_t* EepromHandler::readBytes(EH_dataType _dataType) {
  this->read(_eepromData.payloadByte, _element[_dataType].byteCount, _element[_dataType].location);
  return _eepromData.payloadByte;
}

void EepromHandler::dump() {

  // Just dump the first 256 bytes, that's all we're using at the moment
  
  // I don't know why at this point - maybe I just haven't read the datasheet carefully
  // enough - but, if we try to do all 256 bytes in one call, things go awry around
  // byte 184 (on the CubeCell platform, at least). Splitting the exercise in two seems
  // to resolve whatever issue it is.
   
  this->dump(0,127);
  this->dump(128,255);
}

void EepromHandler::dump(uint8_t _startByte, uint8_t _endByte) {
  if ( _startByte < 0 ) {
    Serial.println(F("[EepromHandler::dump] WARNING: Start byte must not be < 0 (Reset to 0)"));
    _startByte = 0;
  } else if ( _startByte > 255 ) {
    Serial.println(F("[EepromHandler::dump] WARNING: Start byte must not be > 255 (Reset to 255)"));
    _startByte = 255;
  }
  if ( _endByte < 0 ) {
    Serial.println(F("[EepromHandler::dump] WARNING: End byte must not be < 0 (Reset to 0)"));
    _endByte = 0;
  } else if ( _endByte > 255 ) {
    Serial.println(F("[EepromHandler::dump] WARNING: End byte must not be > 255 (Reset to 255)"));
    _endByte = 255;
  }
  uint8_t _eepromByte[256];
  uint8_t* _bytePtr;
  uint8_t _bytesToRead;
  
   _bytesToRead = _endByte - _startByte + 1;
  
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::dump] Start : "));
    Serial.print(_startByte);
    Serial.print(F("; End : "));
    Serial.print(_endByte);
    Serial.print(F("; Bytes : "));
    Serial.println(_bytesToRead);
  }

// It seems we have to read in 32 byte chunks. Anything more seems to get lost with some MCUs.
  
  uint8_t _incrementalStart = _startByte;
  while ( _bytesToRead > 0 ) {
    _bytePtr = &_eepromByte[_incrementalStart];
    if ( _bytesToRead > 32 ) {
      this->read( _bytePtr, 32, _incrementalStart);
      _bytesToRead = _bytesToRead - 32;
      _incrementalStart = _incrementalStart + 32;
    } else {
      this->read( _bytePtr, 32, _incrementalStart);
      _bytesToRead = 0;
    }
  }
  Serial.print(F("[EepromHandler::dump] Dumping bytes "));
  Serial.print(_startByte);
  Serial.print(F(" to "));
  Serial.println(_endByte);
  Serial.println();
  Serial.print(F("       "));
  for ( int i = _startByte; i < _endByte + 1; i++ ) {
    if ( i % 8 == 0 ) {
      if (i < 10) Serial.print(F(" "));
      if (i < 100) Serial.print(F(" "));
      Serial.print(i);
      Serial.print(F("    "));
    }
    if ( _eepromByte[i] < 0x10 ) Serial.print("0");
    Serial.print(_eepromByte[i],HEX);
    if ((i % 8) == 7) {
      Serial.println();
      Serial.print(F("            "));
      for ( int j = i-7; j <= i; j++ ) {
        Serial.print("   ");
        if (_eepromByte[j] > 32 && _eepromByte[j] < 127) {
          Serial.print((char)_eepromByte[j]);
        } else {
          if (_eepromByte[j] < 255) {
            Serial.print("•");
          } else {
            Serial.print("-");
          }
        }
      }
      Serial.println();
      if (i < (_endByte-1)) Serial.print(F("       "));
    } else {
      Serial.print("  ");
    }
  }
  if ((_bytesToRead % 8) != 0) Serial.println();
}

void EepromHandler::scrub() {

  // I don't know why at this point - once again, maybe I just haven't read the datasheet
  // carefully enough - but, if we try to do all 256 bytes in one call, nothing happens
  // (on the CubeCell platform, at least). As with reading (dump()), splitting the
  // exercise in two seems to resolve whatever issue it is.
   
  this->scrub(0,127);
  this->scrub(128,255);
}

void EepromHandler::scrub(uint8_t _startByte, uint8_t _endByte) {

  // Just scrub the first 256 bytes, that's all we're using at the moment
  
  if ( _startByte < 0 ) {
    Serial.println(F("[EepromHandler::scrub] WARNING: Start byte must be > 0 (Reset to 0)"));
    _startByte = 0;
  } else if ( _startByte > 255 ) {
    Serial.println(F("[EepromHandler::scrub] WARNING: Start byte must be < 255 (Reset to 255)"));
    _startByte = 255;
  }
  if ( _endByte < 0 ) {
    Serial.println(F("[EepromHandler::scrub] WARNING: End byte must be > 0 (Reset to 0)"));
    _endByte = 0;
  } else if ( _endByte > 255 ) {
    Serial.println(F("[EepromHandler::scrub] WARNING: End byte must be < 255 (Reset to 255)"));
    _endByte = 255;
  }
  
  uint8_t _allOnes[16];
  for (int i = 0; i < 16; i++) {
    _allOnes[i] = 0xFF;
  }
  uint8_t* _bytePtr;
  uint8_t _bytesToWrite;
  
   _bytesToWrite = _endByte - _startByte + 1;
  
  if ( _EH_DEBUG ) {
    Serial.print(F("[EepromHandler::scrub] Start : "));
    Serial.print(_startByte);
    Serial.print(F("; End : "));
    Serial.print(_endByte);
    Serial.print(F("; Bytes : "));
    Serial.println(_bytesToWrite);
  }
  
  Serial.print(F("[EepromHandler::scrub] Scrubbing bytes "));
  Serial.print(_startByte);
  Serial.print(F(" to "));
  Serial.println(_endByte);
  Serial.println();

// Can only write 16 bytes per write cycle, so do the slicing here

  while ( _bytesToWrite > 0 ) {
    if ( _bytesToWrite > 16 ) {
      this->write(_allOnes, 16, _startByte);
      _bytesToWrite = _bytesToWrite - 16;
      _startByte = _startByte + 16;
    } else {
      this->write(_allOnes, _bytesToWrite, _startByte);
      _bytesToWrite = 0;
    }
  }
}