// mqttTopic.cpp
#include "mqttTopic.h"

mqttTopic::mqttTopic () {
}

void mqttTopic::init() {
  _node = "";
  _type= "";
  _qualifier = "";
}

void mqttTopic::begin(String _topic) {
  init();
  char delimiter = '/';
  unsigned int _length = _topic.length();
  int _i = 0;
  while ((_i < _length) && (_topic.charAt(_i) != delimiter)) {
    _node += _topic.charAt(_i);
    _i++;
  }
  _i++;
  while ((_i < _length) && (_topic.charAt(_i) != delimiter)) {
    _type += _topic.charAt(_i);
    _i++;
  }
  _i++;
  while ((_i < _length) && (_topic.charAt(_i) != delimiter)) {
    _qualifier += _topic.charAt(_i);
    _i++;
  }
}

String mqttTopic::node() {
  return _node;
}

String mqttTopic::type() {
  return _type;
}

String mqttTopic::qualifier() {
  return _qualifier;
}