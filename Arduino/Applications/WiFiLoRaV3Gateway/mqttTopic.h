// mqttTopic.h
#ifndef mqttTopic_h
#define mqttTopic_h

#include <Arduino.h>

class mqttTopic {
  private:
    String _node, _type, _qualifier;
    void init();
  public:
    mqttTopic();
    void begin(String _topic);
    String node();
    String type();
    String qualifier();
};
#endif