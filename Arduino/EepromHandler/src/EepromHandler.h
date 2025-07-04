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

#ifndef EepromHandler_h
#define EepromHandler_h

#include <Arduino.h>
#include <Wire.h>

// Index to EEPROM Data Types

enum EH_dataType {
  EH_GATEWAY_MAC = 0,  // 0
  EH_NODE_MAC,         // 1
  EH_DESCRIPTOR,       // 2
  EH_SEQUENCE,         // 3
  EH_RAINFALL,         // 4
  EH_TANKID,           // 5
  EH_PUMPID,           // 6
  EH_NUM_TYPES
};

class EepromHandler {

  private:
/*
   The following entity defines the storage location and size respectively of an element
   stored in EEPROM
*/    
    struct EH_element {
      const uint8_t location;
      const uint8_t byteCount;
    };

    static constexpr EH_element _element[EH_NUM_TYPES] = {
      {   0,  4},  //  Gateway MAC Address
      {   8,  4},  //  Node MAC Address
      {  16, 48},  //  Descriptor
      { 128,  2},  //  Sequence Number
      { 132,  2},  //  Rainfall Counter
      { 136,  1},  //  Tank ID
      { 138,  1}   //  Pump ID
    };
    static const uint8_t _maxByteCount = 48;  // Largest byteCount in _element array

    static const uint8_t _defaultI2cAddress = 0x50;
    uint8_t _i2cAddress = _defaultI2cAddress;
    bool _EH_DEBUG = false;
    bool _smartSerial = true;
    TwoWire* _i2cBusPtr;

    union _eepromPayload {
      uint8_t payloadByte[_maxByteCount];
      uint32_t macAddress;
      uint16_t counter;
    };
    _eepromPayload _eepromData;

  public:

    EepromHandler();
    
    String softwareRevision();
    
		bool isConnected();
    void init(TwoWire* _wirePtr, uint8_t _address);
    void begin(TwoWire* _wirePtr);
    void begin(TwoWire* _wirePtr, uint8_t _address);
    void begin(TwoWire* _wirePtr, bool _modeFlag);
    void begin(TwoWire* _wirePtr, uint8_t _address, bool _modeFlag);

    void setI2CBus(TwoWire* _wirePtr);
    void setI2CAddress(uint8_t _address);
    uint8_t getI2CAddress();
    bool setSmartSerial();
    void setSmartSerial(bool _value);
    bool getSmartSerial();
    void setDebug(bool _debug);
    bool getDebug();

    uint8_t getParameterByteCount(EH_dataType _dataType);
    uint8_t getParameterLocation(EH_dataType _dataType);
    
    void write(uint8_t* _buf, uint8_t _byteCount, uint8_t _eepromAddress);    
    void writeUint32(EH_dataType _dataType, uint32_t _value);    
    void writeUint16(EH_dataType _dataType, uint16_t _value);    
    void writeUint8(EH_dataType _dataType, uint8_t _value);    
    void writeBytes(EH_dataType _dataType, uint8_t* _buf);    
    void writeBytes(EH_dataType _dataType, uint8_t* _buf, uint8_t _byteCount);
        
    void read(uint8_t* _buf, uint8_t _byteCount, uint8_t _eepromAddress);    
    uint32_t readUint32(EH_dataType _dataType);
    uint16_t readUint16(EH_dataType _dataType);
    uint8_t readUint8(EH_dataType _dataType);
    uint8_t* readBytes(EH_dataType _dataType);
    
    void dump();
    void dump(uint8_t _startByte, uint8_t _endByte);
    void scrub();
    void scrub(uint8_t _startByte, uint8_t _endByte);
    
};

#endif
