/*
    NodeHandler.h - Langlo Node parameters
    Version 0.0.7	23/05/2025

    0.0.1 Initial release
    0.0.2 Unspecified updates
    0.0.3 Add new CubeCell Plus modules (Tank & CubeCell Plus 5)
    0.0.4 Add [(]NodeMCU] Flow Node details
    0.0.5 Add init() with parameters
    0.0.6 Change Node name 'message' when running on Pro Mini
    0.0.7 Add CubeCells 5 and 6 to the Node list

    Digital Concepts
    23 May 2025
    digitalconcepts.net.au

    This is effectively a hosts file, tying host MAC addresses to relevant Node parameters.
    The ultimate intent is that this information would be determined dynamically, through
    some sort of initialisation process when a node is initially powered on.

    Langlo 4-byte LoRa MAC addresses have been allocated by prefixing the byte 0xDC to the
    last three bytes of a WiFi MAC address or Chip ID. I may have to work out something that
    provides a better guarantee of uniqueness but, to date, this has worked out OK.
*/
 
#include "NodeHandler.h"

  // Software Revision Level
  
#ifndef SoftwareRevision_def
  #define SoftwareRevision_def
  struct _SoftwareRevision {
		const uint8_t major;			// Major feature release
		const uint8_t minor;			// Minor feature enhancement release
		const uint8_t minimus;		// 'Bug fix' release
  };
#endif

_SoftwareRevision _nodeHandlerRevNumber = {0,0,6};

NodeHandler::NodeHandler() {
}
    
String NodeHandler::softwareRevision() {
	String _softwareRev;
	_softwareRev = String(_nodeHandlerRevNumber.major) + "." + String(_nodeHandlerRevNumber.minor) + "." + String(_nodeHandlerRevNumber.minimus);
	return _softwareRev;
}

void NodeHandler::begin() {
  init();
}

void NodeHandler::init() {
  NH_heartbeatInterval = 60;		// 1 minute
  NH_healthCheckInterval = 300;	// 5 minutes
  NH_deadbeat = 5;					    // Missed heartbeats before declared dead
}

void NodeHandler::init(uint16_t _heartbeatInterval, uint16_t _healthCheckInterval, uint16_t _deadbeat) {
  NH_heartbeatInterval = _heartbeatInterval;			// in seconds
  NH_healthCheckInterval = _healthCheckInterval;	// in seconds
  NH_deadbeat = _deadbeat;												// Missed heartbeats before declared dead
}

void NodeHandler::setNodeMAC(uint32_t _mac) {
  NH_nodeList[0].mac = _mac;
}

void NodeHandler::setNodeTimer(uint16_t _timer) {
  NH_nodeList[0].timer = _timer;
}

void NodeHandler::setNodeTimerResetValue(uint16_t _timerResetValue) {
  NH_nodeList[0].timerResetValue = _timerResetValue;
}

void NodeHandler::setNodeName(const char * _name) {
  NH_nodeList[0].name = _name;
}

void NodeHandler::setNodeTopic(const char * _topic) {
  NH_nodeList[0].topic = _topic;
}

uint16_t NodeHandler::heartbeatInterval() {
  return NH_heartbeatInterval;
}

void NodeHandler::setHeartbeatInterval(uint16_t _heartbeatInterval) {
  NH_heartbeatInterval = _heartbeatInterval;
}

uint16_t NodeHandler::healthCheckInterval() {
  return NH_healthCheckInterval;
}

void NodeHandler::setHealthCheckInterval(uint16_t _healthCheckInterval) {
  NH_healthCheckInterval = _healthCheckInterval;
}

uint16_t NodeHandler::deadbeat() {
  return NH_deadbeat;
}

void NodeHandler::setDeadbeat(uint16_t _deadbeat) {
  NH_deadbeat = _deadbeat;
}

int NodeHandler::index(uint32_t _sourceMAC) {  
  for (int i = 0; i < NH_nodeCount; i++) {
    if ( NH_nodeList[i].mac == _sourceMAC ) return i;
  }

  // The last element of the array is 'reserved' for the unknown Node
  // If a MAC address does not appear in the list, it is added as the last
  // element. I see now that, the way things are being done now, this will survive
  // the processing of the current packet, which was not the intention—it was meant to
  // just be a temporary store (for some undefined purpose... so maybe I'll just delete
  // this bit in the near future...)
  
  int j = NH_nodeCount - 1;
  NH_nodeList[j].mac = _sourceMAC;
  return j;
}

uint32_t NodeHandler::mac(int _nodeIndex) {
  return NH_nodeList[_nodeIndex].mac;
}

int16_t NodeHandler::timer(int _nodeIndex) {
  return NH_nodeList[_nodeIndex].timer;
}

int16_t NodeHandler::timerResetValue(int _nodeIndex) {
  return NH_nodeList[_nodeIndex].timerResetValue;
}

const String NodeHandler::name(int _nodeIndex) {
  return NH_nodeList[_nodeIndex].name;
}

const String NodeHandler::topic(int _nodeIndex) {
  return NH_nodeList[_nodeIndex].topic;
}

int NodeHandler::nodeCount() {
  return NH_nodeCount;
}

void NodeHandler::incrementTimer(int _nodeIndex) {
  NH_nodeList[_nodeIndex].timer++;
}

void NodeHandler::incrementTimers() {
  for (int i = 0; i < NH_nodeCount; i++) {
    NH_nodeList[i].timer++;
  }
}

void NodeHandler::resetTimer(int _nodeIndex) {
  NH_nodeList[_nodeIndex].timer = NH_nodeList[_nodeIndex].timerResetValue;
}

