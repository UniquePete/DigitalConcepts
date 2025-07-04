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

#ifndef NodeHandler_h
#define NodeHandler_h

#include <Arduino.h>

class NodeHandler {

  private:

/*
Node Data
All hard coded at this stage to simplify system development
*/

// const uint32_t  gatewayMAC = 0xDC6EF174; // Heltec WiFi LoRa 32 Gateway Node 1
// const uint32_t  gatewayMAC = 0xDC6EF75C; // Heltec WiFi LoRa 32 Gateway Node 2
// const uint32_t  gatewayMAC = 0xDC45F8F0; // Heltec WiFi LoRa 32 V2 Gateway Node 1

/*
The Langlo transmission protocol includes a heartbeat process that is used to determine
when a Node is no longer transmitting, as distinct from when messages have simply failed
to reach the gateway, either due to a collision or because the gateway was simply busy.

The nodeRecord struct contains the following elements:
	mac					the Node Langlo/LoRa MAC address
	timer				the current value of the Node heartbeat timer
	timerResetValue		the Node heartbeat timer value used to reset the heartbeat timer
	name				the Node name, generally used in Serial Monitor and Display output
	topic				the Node topic used in MQTT messages
*/

	struct NH_nodeRecord {
		uint32_t mac;
		int16_t timer;
		int16_t timerResetValue;
		const char * name;
		const char * topic;
	};

/*
	I really wanted to use 'const String' rather than 'const char *' in the above for
	consistency (because it was easier to handle Strings elsewhere) but the CubeCell
	implementation hung when attempting to output (completely unrelated data) to a display
	if I did. I can't for the life of me see where the relationship between this
	declaration and the display methods might be. Maybe one day it will reveal itself and
	I can rectify this inconsistency.
*/

// Heartbeat variables

	uint16_t NH_heartbeatInterval;
	uint16_t NH_healthCheckInterval;
	uint16_t NH_deadbeat;

/*
An asynchronous process runs in the background, incrementing the heartbeat timer for each
Node by one, each cycle (NH_heartbeatInterval). Whenever a packet is received from a Node,
its timer is reset to the reset value (Node.timerResetValue).

A second asynchronous process runs checking Node timer values (Node.timer) every
NH_healthCheckInterval. If a timer is greater than the NH_deadbeat value, it will be
declared 'dead' and an appropriate message will be sent to the MQTT broker to flag this
condition.

With the above [default] values for the heartbeat timers, the timer reset value for a Node
transmitting every 60 seconds should be set to zero. This will allow for five missed
packets before the Node is declared dead.

For a Node transmitting once every five minutes, to allow for the same level of
tolerance (i.e. five missed packets), the timer reset value would need to be set to -20,
since the Node timer would be incremented five times in each transmission period.

If, however, all Nodes are transmitting at five minute intervals, the Heartbeat Interval
can be set to 300 (seconds), the Health Check Interval to 1500 (seconds) and the Node
timer reset values set to zero.
*/

#if defined(__AVR_ATmega328P__)
	static const uint8_t NH_nodeCount = 1; // For the Pro Mini, just itself
#else
	static const uint8_t NH_nodeCount = 51; // The number of Nodes in the following list
#endif

/*
The following list must include the 'Unknown Node', which should be the last entry in
the list. This location is used as a temporary store for details of any Node that is
not recognised.

We need to put as little as possible in the Pro Mini because it is severely memory
constrained—only 2K bytes for variables. We need to keep globals usage down to ~1500 bytes
*/

	NH_nodeRecord NH_nodeList[NH_nodeCount] = {
#if !defined(__AVR_ATmega328P__)
		{ 0xDC12051B, 0, 0, "Pro Mini 1"              , "promini1"      },	// 0
		{ 0xDCE00002, 0, 0, "Pro Mini 2"              ,	"promini2"      },	// 1
		{ 0xDC11142C, 0, 0, "Wijnand3"                ,	"porch"         },	// 2
		{ 0xDC112316, 0, 0, "Wijnand4"                ,	"shed"          },	// 3
		{ 0xDC112B0D, 0, 0, "Pro Mini 5"              ,	"promini5"      },	// 4
		{ 0xDC12111D, 0, 0, "Pro Mini 6"              ,	"promini6"      },	// 5
		{ 0xDC121123, 0, 0, "Pro Mini 7"              ,	"promini7"     	},	// 6
		{ 0xDC120619, 0, 0, "Pro Mini 8"              ,	"promini8"     	},	// 7
		{ 0xDC6EF174, 0, 0, "WiFi LoRa Gateway 1"	    ,	"gateway1"		  },	// 8
		{ 0xDC6EF75C, 0, 0, "WiFi LoRa Gateway 2"	    ,	"gateway2"	    },	// 9
		{ 0xDC45f8f0, 0, 0, "WiFi LoRa V2 Gateway 1"	,	"gateway21"	    },	// 10
		{ 0xDC460914, 0, 0, "WiFi LoRa V2 Gateway 2"	,	"gateway22"	    },	// 11
		{ 0xDC80649C, 0, 0, "WiFi LoRa V3 Gateway 1"	,	"gateway31"	    },	// 12
		{ 0xDC815740, 0, 0, "WiFi LoRa V3 Gateway 2"	,	"gateway32"	    },	// 13
		{ 0xDC125398, 0, 0, "Elecrow ESP32 1"					, "study"			    },	// 14
		{ 0xDC125784, 0, 0, "Elecrow ESP32 2"         ,	"rover"        	},	// 15
		{ 0xDC517110, 0, 0, "Wireless Stick One" 		  ,	"raingauge"    	},	// 16
		{ 0xDC51739C, 0, 0, "Wireless Stick Two" 		  ,	"tank"         	},	// 17
		{ 0xDC822940, 0, 0, "Wireless Stick Three" 	  ,	"wslv31"        },	// 18
		{ 0xDC822D9C, 0, 0, "Wireless Stick Four" 	  ,	"wslv32"        },	// 19
		{ 0xDC032F31, 0, 0, "CubeCell One"       		  ,	"cubecell1"     },	// 20
		{ 0xDC531410, 0, 0, "CubeCell Two"        		,	"cubecell2"     },	// 21
		{ 0xDC632207, 0, 0, "CubeCell Three"        	,	"cubecell3"     },	// 22
		{ 0xDC772E37, 0, 0, "CubeCell Four"        	  ,	"cubecell4"     },	// 23
		{ 0xDC772E41, 0, 0, "CubeCell Five"        	  ,	"cubecell5"     },	// 24
		{ 0xDC77330F, 0, 0, "CubeCell Six"        	  ,	"cubecell6"     },	// 25
		{ 0xDC296D39, 0, 0, "ESP12S"         			    ,	"esp12s"       	},	// 26
		{ 0xDCB600EA, 0, 0, "ESP12F1"        			    ,	"esp12F1"      	},	// 27
		{ 0xDCB61052, 0, 0, "ESP12F2"        			    ,	"esp12F2"      	},	// 28
		{ 0xDC2EDF35, 0, 0, "ESP12F3"        			    ,	"esp12F3"      	},	// 29
		{ 0xDC2EE2F7, 0, 0, "ESP12F4"         	      ,	"esp12F4"      	},	// 30
		{ 0xDC29D4DE, 0, 0, "ESP12F5"                 , "esp12F5"       },  // 31
		{ 0xDC2B5D56, 0, 0, "ESP12F6"                 , "esp12F6"       },  // 32
		{ 0xDCA541E3, 0, 0, "Node21"         			    ,	"node21"       	},	// 33
		{ 0xDCB00002, 0, 0, "Node22"         			    ,	"node22"       	},	// 34
		{ 0xDC120F38, 0, 0, "Node23"         			    ,	"node23"       	},	// 35
		{ 0xDCB5A14B, 0, 0, "Node24"         			    ,	"node24"       	},	// 36
		{ 0xDC8DBAAC, 0, 0, "NodeMCU Flow Sensor"    	,	"flownode"      },	// 37
		{ 0xDC6A5728, 0, 0, "ESP32 W1"        			  ,	"esp32w1"      	},	// 38
		{ 0xDC6A56EC, 0, 0, "ESP32 W2"        			  ,	"esp32w2"      	},	// 39
		{ 0xDC48870C, 0, 0, "ESP32 W3"        			  ,	"esp32w3"      	},	// 40
		{ 0xDC488D6C, 0, 0, "ESP32 W4"        			  ,	"esp32w4"      	},	// 41
		{ 0xDC488710, 0, 0, "ESP32 W5"         			  ,	"esp32w5"     	},	// 42
		{ 0xDC4886E8, 0, 0, "Node36"         			    ,	"node36"       	},	// 43
		{ 0xDC4886F4, 0, 0, "Node37"         			    ,	"node37"       	},	// 44
		{ 0xDCA7290F, 0, 0, "CubeCell Plus Pump"    	,	"cellpluspump"  },	// 45
		{ 0xDCB32836, 0, 0, "Weather Station"  			  ,	"solarcellplus"	},	// 46
		{ 0xDCB32A0B, 0, 0, "AWTS"  					        ,	"thirdcellplus"	},	// 47
		{ 0xDC77172A, 0, 0, "Tank 1"  					      ,	"tankplus1"	    },	// 48
		{ 0xDC770630, 0, 0, "CubeCell Plus Five"  	  ,	"cellplus5"   	},	// 49
#endif		
		{ 0x0       , 0, 0, "Name not available"  		,	"unknown"       },	// 50
	};
  
  public:

    NodeHandler();
    
    String softwareRevision();
    
    void init();
    void init(uint16_t _heartbeatInterval, uint16_t _healthCheckInterval, uint16_t _deadbeat);
    void begin();

    void setNodeMAC(uint32_t _mac);
    void setNodeTimer(uint16_t _timer);
    void setNodeTimerResetValue(uint16_t _timerResetValue);
    void setNodeName(const char * _name);
    void setNodeTopic(const char * _topic);
    
    uint16_t heartbeatInterval();
    void setHeartbeatInterval(uint16_t _heartbeatInterval);
    uint16_t healthCheckInterval();
    void setHealthCheckInterval(uint16_t _healthCheckInterval);
    uint16_t deadbeat();
    void setDeadbeat(uint16_t _deadbeat);

    int	index(uint32_t _sourceMac);
    uint32_t mac(int _nodeIndex);
    int16_t timer(int _nodeIndex);
    int16_t timerResetValue(int _nodeIndex);
    const String name(int _nodeIndex);
    const String topic(int _nodeIndex);
    int nodeCount();
    void incrementTimer(int _nodeIndex);
    void incrementTimers();
    void resetTimer(int _nodeIndex);
};

#endif
