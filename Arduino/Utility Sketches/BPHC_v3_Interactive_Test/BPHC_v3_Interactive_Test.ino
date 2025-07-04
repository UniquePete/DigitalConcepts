/*  Test the functions of the BPHC board. This sketch also effectively provides code examples
    for the use of each of these functions.
  
    Tests:
       1. ACS712 sensor
       2. BME280 sensor
       3. Battery voltage
       4. DS18B20 sensor
       5. EEPROM read
       6. Local/Remote Mode Button press/release interrupts
       7. LoRa messaging
       8. Local/Remote Mode LED, through PCA9536
       9. CubeCell NeoPixel
      10. Relay control and ON/OFF LEDs
      11. Scan I2C bus
      12. SD Card read/write
      13. Software revision levels
      14. Vext control and Vext LED
      15. Watchdog timer operation

    EEPROM content is written independently using the ResetEEPROM_EH.ino sketch.

    24 May 2025 1.0.0 Base release
    26 May 2025 1.1.0 Refactor input processing code (processInput())
    27 May 2025 1.1.1 Improve consistency in variable naming and comments, and general tidy up

    27 May 2025
    Digital Concepts
    www.digitalconcepts.net.au
*/
 
#include <CubeCell_NeoPixel.h>  // CubeCell NeoPixel control library
#include <OneWire.h>            // OneWire bus
#include <DallasTemperature.h>  // DS18B20
#include <Wire.h>               // I2C bus
#include <Seeed_BME280.h>       // BME280
#include <SPI.h>                // SPI bus
#include <SD.h>                 // SD Card reader
#include <LoRa_APP.h>           // LoRa

#include "LangloLoRa.h"         // LoRa configuration parameters
#include "EepromHandler.h"      // EEPROM Handler class with access methods
#include "PacketHandler.h"      // Packet Handler class with access methods

//#include "CubeCellPins.h"
#include "CubeCellV2Pins.h"

// BPHC-specific pin definitions
#define WDT_DONE       GPIO0   // Watchdog Timer Done signal
#define ONE_WIRE_BUS   GPIO1   // DS18B20 OneWire bus
#define MODE_BUTTON    GPIO2   // Interrupt button on BPHC PCB
#define SPI_SD_CS      GPIO3   // SD Card reader SPI CS
//                     GPIO4   // CubeCell NeoPixel
#define RELAY_CONTROL  GPIO5   // Relay control

// Sketch revision level
struct softwareRevision {
  const uint8_t major;			// Major feature release
  const uint8_t minor;			// Minor feature enhancement release
  const uint8_t minimus;		// 'Bug fix' release
};
softwareRevision sketchRevision = {1,1,1};

// The following are the Serial Monitor inputs that will be recognised
const String Test_Current   = "AC"; // ACS712 current sensor
const String Test_BME280    = "BM"; // BME280 atmospheric sensor
const String Test_Voltage   = "BV"; // Battery Voltage
const String Test_DS18B20   = "DS"; // DS18B20 temperature sensor
const String Test_EEPROM    = "ER"; // EEPROM Read
const String Test_Interrupt = "IB"; // Interrupt Button
const String Test_LoRa      = "LM"; // LoRa Message
const String Test_Mode      = "MC"; // Local/Remote Mode Control
const String Test_Neo       = "NP"; // CubeCell NeoPixel
const String Test_Relay     = "RC"; // Relay Control
const String Test_Scan      = "SC"; // Scan the I2C bus
const String Test_SDCard    = "SD"; // SD Card reader operation
const String Test_Revision  = "SR"; // Software Revision levels
const String Test_Vext      = "VE"; // Vext control
const String Test_WatchDog  = "WT"; // Watchdog Timer
const String returnChar     = "";   // Carriage Return

const String ON_State       = "ON";
const String OFF_State      = "OF";
const String Delete_Option  = "D";
const String Exists_Option  = "E";
const String Read_Option    = "R";
const String Write_Option   = "W";

String inputString;

// ACS712 current sensor
  // Nothing special here

// BME280 atmospheric sensor
const uint8_t I2C_BME280_Address[]  = {0x76,0x77};

uint16_t humidity = 0;
uint16_t pressure = 0;
int16_t temperature = 0;

BME280 bme280;

// CubeCell battery voltage measurement
  // Nothing special here

// DS18B20 temperature sensor
#define TEMPERATURE_PRECISION 9

int nodeTemperature = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20Sensor(&oneWire);

// EEPROM
const uint8_t I2C_EEPROM_Address    = 0x50;

bool smartSerial = false;       // Smart Serial Addressing required for 32K and 64K EEPROMs

uint32_t gatewayMAC = 0;
uint32_t nodeMAC = 0;
uint8_t* descriptor;
uint16_t sequenceNumber, rainfallCounter;
uint8_t tankId, pumpId;

EepromHandler eeprom;

// Interrupt button
// Commentary on the subject suggests that variables used within an ISR should be declared as volatile
volatile bool pressFlag = false;
volatile bool releaseFlag = false;

// LoRa
static RadioEvents_t RadioEvents;
void onTxDone( void );
void onTxTimeout( void );
int16_t rssi,rxSize;

uint16_t messageCounter = 0;

PacketHandler packet;

// Mode Control LED (PCA9536)
const uint8_t I2C_PCA9536_Address   = 0x41;
// PCA9536 registers
#define PCA9536_INPUT_PORT    0x00
#define PCA9536_OUTPUT_PORT   0x01
#define PCA9536_POLARITY_INV  0x02
#define PCA9536_CONFIG        0x03

// NeoPixel
typedef enum {
  NP_OFF,
  NP_RED,
  NP_GREEN,
  NP_BLUE
} neoPixelColour_t;
// NeoPixel Parameters: # pixels, RGB or RGBW device, device colour order + frequency
CubeCell_NeoPixel neo(1, RGB, NEO_GRB + NEO_KHZ800);

// Relay
  // Nothing special here

// SD Card reader
  // Nothing special here

// Vext
bool vextOnFlag = false;

// Watchdog timer
uint16_t feedingInterval;
static TimerEvent_t watchdogFeeder;

void setup() { 
  Serial.begin(115200);
  Serial.println("[setup] BPHC PCB Test Program");
  printSoftwareRevision();

// Interrupt button
  PINMODE_INPUT_PULLUP(MODE_BUTTON);

// LoRa
  RadioEvents.TxDone = onTxDone;
  RadioEvents.TxTimeout = onTxTimeout;

  Radio.Init( &RadioEvents );
  Radio.SetChannel( RF_FREQUENCY );
  Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                 LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                 LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                 true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );
  Radio.Sleep( );
  
// Relay control
  pinMode(RELAY_CONTROL,OUTPUT);

// SD Card reader
  pinMode(SPI_SD_CS,OUTPUT);

// Vext
  pinMode(Vext,OUTPUT);
  vextOff();

// Watchdog timer
/*
  The Watchdog Timer is used to 'feed' the watchdog at preset intervals. The watchdog
  must be fed more often that its [hardware] preset timeout interval. The feeding interval,
  <wdtLimit> milliseconds, can be set within the watchdog test sequence. By default, the
  'feeder' is inactive and the watchdog will reset the Node when its timeout interval expires.
*/
  pinMode(WDT_DONE,OUTPUT);
  TimerInit(&watchdogFeeder, feedTheDog);

  promptForInput();
}

void loop() {
  if ( Serial.available() > 0 ) {
    inputString = Serial.readString();
    Serial.print("[loop] Input string: ");
    Serial.println(inputString);
    processInput(inputString);
    promptForInput();
  }
}

// Sketch flow management

void promptForInput() {
  Serial.println();
  Serial.println("Enter  AC to read and display ACS712 current sensor output");
  Serial.println("       BM to read and display BME280 atmospheric sensor output");
  Serial.println("       BV to read and display the battery voltage (not the ture value under USB power)");
  Serial.println("       DS to read and display DS18B20 temperature sensor output");
  Serial.println("       ER to read and display EEPROM content");
  Serial.println("       IB to test the operation of the Local/Remote Mode interrupt button");
  Serial.println("       LM to send a LoRa message");
  Serial.println("       MC to switch Mode Control LED (PCA9536 Port 0) ON or OFF");
  Serial.println("       NP to light the CubeCell NeoPixel");
  Serial.println("       RC to switch the relay and associated LEDs ON or OFF");
  Serial.println("       SC to scan the I2C bus");
  Serial.println("       SD to write and read data to/from the SD Card reader");
  Serial.println("       SR to display software revision levels");
  Serial.println("       VE to switch Vext, and the Vext LED, ON or OFF");
  Serial.println("       WT to test the watchdog timer");
  Serial.println();
}

void processInput( String inputString ) {

  int stringLength = inputString.length();
  String upperString = inputString;
  upperString.toUpperCase();

	//  Serial.println("[processInput] Skip leading white space...");
  int i = 0;
  while ( isspace(upperString[i]) ) i++;
	//  Serial.println("[processInput] Process command...");
  if ( i + 2 < stringLength ) {
		String command = upperString.substring(i,i+2);
    i = i+2;
		if ( command == Test_Current ) {
	//    Serial.println("Identifier is AC");
			readACS712Sensor();
		} else if ( command == Test_BME280 ) {
	//    Serial.println("Identifier is BM");
			readBME280Sensor();
		} else if ( command == Test_Voltage ) {
	//    Serial.println("Identifier is BV");
			readBatteryVoltage();
		} else if ( command == Test_DS18B20 ) {
	//    Serial.println("Identifier is DS");
			readDS18B20Sensor();
		} else if ( command == Test_EEPROM ) {
	//    Serial.println("Identifier is ER");
			readEepromContent();
		} else if ( command == Test_Interrupt ) {
	//    Serial.println("Identifier is IB");
			// Get number of cycles and long press duration from input string.
      // If none specified, cycles defaults to 5 and longPressDuration to 1000 miliseconds.
			int cycleCount = 5;
      unsigned long longPressDuration = 1000;
			while ( !isdigit(upperString[i]) && i < stringLength ) i++;
			if ( i < stringLength ) {
				int j = i;
				while ( isdigit(upperString[j]) && j < stringLength ) j++;
				cycleCount = upperString.substring(i,j).toInt();
				int k = j;
        while ( !isdigit(upperString[k]) && k < stringLength ) k++;
        if ( k < stringLength ) {
          int l = k;
          while ( isdigit(upperString[l]) && l < stringLength ) l++;
          longPressDuration = upperString.substring(k,l).toInt();
        }
			}
			processInterrupts(cycleCount,longPressDuration);
		} else if ( command == Test_LoRa ) {
	//    Serial.println("Identifier is LM");
			while ( isspace(upperString[i]) ) i++;
			if ( i < stringLength ) {
				// Provide any appropriate options...
			} else {
				// Just send a Reset packet
				Serial.println("[Test_LoRa] Sending packet, Reset Code 99...");
				sendLoraMessage();
			}
		} else if ( command == Test_Mode ) {
	//    Serial.println("Identifier is MC");
			int port = 0;
			while ( isspace(upperString[i]) ) i++;
			if ( i < stringLength ) {
				if ( isdigit(upperString[i]) ) {
					port = upperString.substring(i,i+1).toInt();
					if ( port > 3 ) {
						Serial.println("[Test_Mode] Port must be in the range 0..3");
					} else {
						i++;
						while ( isspace(upperString[i]) ) i++;
						if ( i < stringLength ) {
							String state = upperString.substring(i,i+2);
							if ( state == ON_State ) {
								pcaPortOn(port);
							} else if ( state == OFF_State ) {
								pcaPortOff(port);
							} else {
								Serial.println("[Test_Mode] State not recognised - Command format: MC <port #> <ON/OFF>");
							}
						} else {
							Serial.println("[Test_Mode] No State entered - Command format: MC <port #> <ON/OFF>");
						}
					}
				} else {
					Serial.println("[Test_Mode] Command format: MC <port #> <ON/OFF>");
				}
			} else {
				Serial.println("[Test_Mode] Command format: MC <port #> <ON/OFF>");
			}
		} else if ( command == Test_Neo ) {
	//    Serial.println("Identifier is NP");
			while ( isspace(upperString[i]) ) i++;
			if ( i < stringLength ) {
				// See what colour they want...
				// Create an array of colours to search for
				String colours[] = {"RED", "GREEN", "BLUE", "OFF"};      
				// Initialize minimum position with a value larger than string length
				int minPos = stringLength + 1;
				String foundColour = "";      
				// Search for each colour
				for (String colour : colours) {
						int pos = upperString.indexOf(colour);
						if (pos != -1 && pos < minPos) {
								minPos = pos;
								foundColour = colour;
						}
				}
				if ( foundColour.length() > 0 ) {
					if ( foundColour == "RED" ) {
						Serial.println("[Test_Neo] NeoPixel RED...");
						neoPixel(NP_RED);
					} else if ( foundColour == "GREEN" ) {
						Serial.println("[Test_Neo] NeoPixel GREEN...");
						neoPixel(NP_GREEN);
					} else if ( foundColour == "BLUE" ) {
						Serial.println("[Test_Neo] NeoPixel BLUE...");
						neoPixel(NP_BLUE);
					} else {
						Serial.println("[Test_Neo] NeoPixel OFF...");
						neoPixel(NP_OFF);
					}
				} else {
					Serial.println("[Test_Neo] Usage : NP RED/GREEN/BLUE/OFF");
				}
			} else {
				// Just cycle the NeoPixel through the three primaries
				Serial.println("[Test_Neo] Cycling NeoPixel...");
				cycleNeoPixel();
				Serial.println("[Test_Neo] Cycle complete");
			}
		} else if ( command == Test_Relay ) {
	//    Serial.println("Identifier is RC");
			while ( isspace(upperString[i]) ) i++;
			String state = upperString.substring(i,i+2);
			if ( state == ON_State ) {
				Serial.println("[Test_Relay] Relay ON");
				digitalWrite(RELAY_CONTROL,HIGH);     // Turn on the external power supply
			} else if ( state == OFF_State ) {
				Serial.println("[Test_Relay] Relay OFF");
				digitalWrite(RELAY_CONTROL,LOW);     // Turn off the external power supply
			} else {
				Serial.println("[Test_Relay] Command not recognised, use ON or OFF");
			}
		} else if ( command == Test_Scan ) {
	//    Serial.println("Identifier is SC");
			uint8_t deviceAddress;
			int k = inputString.indexOf("0x");
			if (k < 0) {
				Serial.println("[Test_Scan] No hex prefx found, check for a decimal number...");
				// No HEX prefix found, so just look for a string of [decimal] digits
				while ( !isdigit(inputString[i]) && i < stringLength ) i++;
				if ( i < stringLength ) {
					int j = i;
					while ( isdigit(inputString[j]) && j < stringLength ) j++;
					Serial.print("[Test_Scan] Found : ");
					Serial.println(inputString.substring(i,j));
					deviceAddress = inputString.substring(i,j).toInt();
					scanI2cBus(deviceAddress);
				} else {
					Serial.println("[Test_Scan] No address found, just scan the bus");
					scanI2cBus(0);
				}
			} else {
				// Parse out the HEX digits
				Serial.println("[Test_Scan] Parsing hex string...");
				i = k + 2;
				int j = i;
				while (isHexadecimalDigit(inputString[j]) && j < stringLength ) j++;
				String hexString = inputString.substring(i,j);
				deviceAddress = strtoul(hexString.c_str(),0,16);
				Serial.print("[Test_Scan] Found : 0x");
				Serial.println(inputString.substring(i,j));
				scanI2cBus(deviceAddress);
			}
		} else if ( command == Test_SDCard ) {
	//    Serial.println("Identifier is SD");
			while ( isspace(upperString[i]) ) i++;
			if ( i < stringLength ) {
				String option = upperString.substring(i,i+1);
				if ( option == Delete_Option ) {
					deleteSdCardFile();
				} else if ( option == Exists_Option ) {
					existsSdCardFile();
				} else if ( option == Read_Option ) {
					readSdCardFile();
				} else if ( option == Write_Option ) {
					writeSdCardFile();
				} else {
					Serial.println("[Test_SDCard] Command option not recognised, use [D]elete, [E]xists, [R]ead or [W]rite");
				}
			} else {
				// Just display the SD card specs
				getSdCardInfo();
			}
		} else if ( command == Test_Revision ) {
	//    Serial.println("Identifier is SR");
			printSoftwareRevision();
		} else if ( command == Test_Vext ) {
	//    Serial.println("Identifier is VC");
			while ( isspace(upperString[i]) ) i++;
			String state = upperString.substring(i,i+2);
			if ( state == ON_State ) {
				vextOn();     // Turn on the external power supply
			} else if ( state == OFF_State ) {
				vextOff();    // Turn off the external power supply
			} else {
				Serial.println("[Test_Vext] Command not recognised, use ON or OFF");
			}
		} else if ( command == Test_WatchDog ) {
	//    Serial.println("Identifier is WT");
			while ( !isdigit(upperString[i]) && i < stringLength ) i++;
			if ( i < stringLength ) {
				int j = i;
				while ( isdigit(upperString[j]) && j < stringLength ) j++;
				feedingInterval = upperString.substring(i,j).toInt();
				setWatchdogTimer(feedingInterval);
			} else {
				Serial.println("[Test_WatchDog] The watchdog timeout is set in hardware, via the resistance set through trimpot R15.");
				Serial.println("                We need to feed the dog regularly, before the timeout timer expires. If we let this");
				Serial.println("                program run for several minutes, the processor will reset when the timer expires. If");
				Serial.println("                we note the time interval between resets, we can determine how often we need to feed");
				Serial.println("                the dog. The feeding interval, specified in milliseconds, should be less than the");
				Serial.println("                timeout interval by some appropriate 'safety' margin."); 
				Serial.println("                To set a feeding interval, enter: WD <interval>");
				Serial.println();
				if ( feedingInterval == 0 ) {
					Serial.println("[Test_WatchDog] No feeding interval is currently set");
				} else {
					Serial.print("[Test_WatchDog] The feeding interval is currently set at ");
					Serial.print(feedingInterval);
					Serial.println(" seconds");
				}
			}
		}
  } else {
  	Serial.println("[processInput] Command not recognised...");
  }
}

bool isHexadecimalDigit( char character ) {
  return  ( character >= '0' && character <= '9') ||
          ( character >= 'A' && character <= 'F') ||
          ( character >= 'a' && character <= 'f');
}

// ACS712

void readACS712Sensor() {
  Serial.println("[readACS712Sensor] Display sensor reading...");
  Serial.println("                   Coming soon...");
}

// BME280

void readBME280Sensor() {
  bool OKtoGO = false;
  Serial.println("[readBME280Sensor] Read sensor...");
  delay(50);                  // Give everything a moment to settle down
  Wire.begin();               // On with the show...
  
  Serial.println("[readBME280Sensor] Check possible BME280 sensor addresses...");
  Serial.print("[readBME280Sensor] Try 0x");
  Serial.print(I2C_BME280_Address[0],HEX);
  Serial.println("...");
  if (bme280.init(I2C_BME280_Address[0])) {
    Serial.println("[readBME280Sensor] Sensor found");
    OKtoGO = true;
  } else {
    Serial.print("[readBME280Sensor] Try 0x");
    Serial.print(I2C_BME280_Address[1],HEX);
    Serial.println("...");
    if (bme280.init(I2C_BME280_Address[1])) {
      Serial.println("[readBME280Sensor] Sensor found");
      OKtoGO = true;
    } else {
      Serial.println( "[readBME280Sensor] Cannot find BME280 sensor, check Vext.=" );
    }
  }
  
  if (OKtoGO) {
    Serial.println( "[readBME280Sensor] BME280 sensor initialisation complete" );

	  delay(100); // The BME280 needs a moment to get itself together... (50ms is too little time)

	  temperature = (int) (10*bme280.getTemperature());
	  pressure = (int) (bme280.getPressure() / 91.79F);
	  humidity = (int) (bme280.getHumidity());

		Serial.println();
		Serial.print("[readBME280Sensor] Temperature: ");
		Serial.println((float) temperature/10, 1);
		Serial.print("[readBME280Sensor] Pressure: ");
		Serial.println(pressure);
		Serial.print("[readBME280Sensor] Humidity: ");
		Serial.println(humidity);
  }

  Wire.end();
}

// Battery voltage

void readBatteryVoltage() {
  uint16_t batteryVoltage = getBatteryVoltage();
  Serial.print("[readBatteryVoltage] Battery Voltage : ");
  Serial.print( batteryVoltage );
  Serial.println(" mV");
}

// DS18B20

void readDS18B20Sensor() {
  Serial.println("[readDS18B20Sensor] Read sensor...");
/*
 * If this node is reading atmospheric conditions from a BME sensor
 * All values recorded as integers, temperature multiplied by 10 to keep one decimal place
*/
  ds18b20Sensor.begin();
  delay(100);                 // Give everything a moment to settle down
  if ( ds18b20Sensor.getDeviceCount() > 0 ) {
    ds18b20Sensor.requestTemperatures(); // Send the command to get temperatures
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and get the temperature from the first sensor only.
    float tempTemperature = ds18b20Sensor.getTempCByIndex(0);
    Serial.print("[readDS18B20Sensor] Returned value: ");
    Serial.println(tempTemperature);

    nodeTemperature = (int) (10*(tempTemperature + 0.05));
  
    Serial.print("[readDS18B20Sensor] Temperature: ");
    Serial.println((float) nodeTemperature/10, 1);
  } else {
    Serial.println("[readDS18B20Sensor] No sensor found, check Vext");
  }
}

// EEPROM

void readEepromContent() {
  Wire.begin();
  eeprom.begin(&Wire);
  if ( eeprom.isConnected() ) {
    printEepromContent();
  } else {
    Serial.println("[readEepromContent] EEPROM not found, check Vext");
  }
  Wire.end();
}

void printEepromContent() {
  smartSerial = eeprom.setSmartSerial();
  if (smartSerial) {
    Serial.println("[printEepromContent] Smart Serial Addressing (32K+ EEPROM)");
  } else {
    Serial.println("[printEepromContent] Standard Serial Addressing (16K- EEPROM)");
  }
  gatewayMAC = eeprom.readUint32(EH_GATEWAY_MAC);
  Serial.print(F("   Gateway MAC (GM): 0x"));
  Serial.println(gatewayMAC,HEX);
  nodeMAC = eeprom.readUint32(EH_NODE_MAC);
  Serial.print(F("      Node MAC (NM): 0x"));
  Serial.println(nodeMAC,HEX);
  Serial.print(F("    Descriptor (DS): "));
  descriptor = eeprom.readBytes(EH_DESCRIPTOR);
  int byteCount = eeprom.getParameterByteCount(EH_DESCRIPTOR);
  for (int i = 0; i < byteCount; i++) {
    Serial.print((char)descriptor[i]);
  }
  Serial.println();
  sequenceNumber = eeprom.readUint16(EH_SEQUENCE);
  Serial.print(F("    Sequence # (SN): "));
  Serial.println(sequenceNumber);
  rainfallCounter = eeprom.readUint16(EH_RAINFALL);
  Serial.print(F("  Rain Counter (RC): "));
  Serial.println(rainfallCounter);
  tankId = eeprom.readUint16(EH_TANKID);
  Serial.print(F("       Tank ID (TD): "));
  Serial.println(tankId);
  pumpId = eeprom.readUint16(EH_PUMPID);
  Serial.print(F("       Pump ID (PD): "));
  Serial.println(pumpId);
}

// Interrupts

void buttonPress() {
  pressFlag = true;
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON), buttonRelease, RISING);
//  Serial.println("Press ping!");
}

void buttonRelease() {
  releaseFlag = true;
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON), buttonPress, FALLING);
//  Serial.println("Release ping!");
}

void processInterrupts( uint8_t interruptCount, unsigned long longPressDuration ) {
  /* A button press pulls the interrupt pin to ground, so the leading edge  */
  /* of the interrupt will be falling, while the trailing edge is rising.   */
  Serial.print("[processInterrupts] Initiate interrupt test, ");
  Serial.print(interruptCount);
  Serial.print(" cycles with a long press duratiion of ");
  Serial.print(longPressDuration);
  Serial.println(" milliseconds...");
  Serial.println("[processInterrupts] Press the MODE button to generate an interrupt...");
  Serial.println();

  // Ultimately we want this section to also control the relevant LEDs and the relay
  // Long Press toggles between Local and Remote Modes, turning the Mode LED ON and OFF accordingly
  // Short Press toggles the relay, and ON/OFF LEDs, when in Local Mode

  bool pressProcess = false;
  bool releaseProcess = true;

  pressFlag = false;
  attachInterrupt(digitalPinToInterrupt(MODE_BUTTON), buttonPress, FALLING);
  unsigned long timeInterval;
  unsigned long thenTime = 0;
  unsigned long nowTime = 0;
  unsigned long debounceInterval = 200;
  int i = 0;
  while ( i < interruptCount ) {
    if ( pressFlag ) {
      if ( releaseProcess ) {
        Serial.println("[processInterrupts] Button press [FALLING interrupt] detected");
        thenTime = millis();
        timeInterval = thenTime - nowTime;
        Serial.print("[processInterrupts] nowTime : ");
        Serial.print(nowTime);
        Serial.print("   thenTime : ");
        Serial.print(thenTime);
        Serial.print("   Interval : ");
        Serial.print(timeInterval);
        if ( timeInterval < debounceInterval ) {
          Serial.println("  Bounce...");
          Serial.println("[processInterrupts] Bounce detected, reset interrupts...");
          Serial.println();
          delay(100);
          attachInterrupt(digitalPinToInterrupt(MODE_BUTTON), buttonPress, FALLING);
          releaseFlag = false;
          pressProcess = false;
          releaseProcess = true;
        } else {
          Serial.println();
          pressProcess = true;
          releaseProcess = false;
        }
      }
      pressFlag = false;
    }
    if ( releaseFlag ) {
      if ( pressProcess ) {
        Serial.println("[processInterrupts] Button release [RISING interrupt] detected");
        nowTime = millis();
        timeInterval = nowTime - thenTime;
        Serial.print("[processInterrupts] thenTime : ");
        Serial.print(thenTime);
        Serial.print("   nowTime : ");
        Serial.print(nowTime);
        Serial.print("   Interval : ");
        Serial.print(timeInterval);
        if ( timeInterval > longPressDuration ) {
          Serial.println("  Long Press...");
        } else {
          Serial.println("  Short Press...");
        }
        Serial.println();
        pressProcess = false;
        releaseProcess = true;
        i++;
      }
      thenTime = nowTime;
      releaseFlag = false;
    }
    /* For some reason, we have to have this trivial delay here. Without it, the above serial output  */
    /* never appears [when the relevant flags are set]. It's no good inside the if statement, and     */
    /*  it's no good if it's not here, but I'll be stuffed if I know why it's required...             */
    delay(1);
  }
  detachInterrupt( digitalPinToInterrupt(MODE_BUTTON));
  Serial.println("[processInterrupts] OK, next test.");
}

// LoRa

void onTxDone(void) {
  Radio.Sleep();
}
void onTxTimeout(void) {
  Radio.Sleep();
}

void sendLoraMessage() {
  packet.begin(gatewayMAC, nodeMAC, messageCounter);
  packet.setPacketType(RESET);
  packet.setResetCode(99);
  int totalByteCount = packet.packetByteCount();
  Serial.println("[sendLoraMessage] Finished Packet");
  neoPixel(NP_GREEN);   // NeoPixel [low intensity] GREEN
  Radio.Send((uint8_t *)packet.byteStream(), packet.packetByteCount());
  neoPixel(NP_OFF);    // NeoPixel OFF
  Serial.println("[sendLoraMessage] Packet Sent");
  Serial.println();
}

// PCA9536 - Local/Remote Mode LED

void pcaPortOn(uint8_t port) {
  Serial.print("[pcaPortOn] Switching output port ");
  Serial.print(port); 
  Serial.println(" ON");

  // Initialize I2C
  byte result;
  Wire.begin();
  
  // Configure PCA9536 as outputs
  Wire.beginTransmission(I2C_PCA9536_Address);
  Wire.write(PCA9536_CONFIG);
  Wire.write(0x00); // 0x00 = all outputs
  result = Wire.endTransmission();

  if ( result == 0 ) {
    // Set polarity inversion to normal
    Wire.beginTransmission(I2C_PCA9536_Address);
    Wire.write(PCA9536_POLARITY_INV);
    Wire.write(0x00); // 0x00 = normal polarity
    Wire.endTransmission();

    byte controlByte = 1 << port;
  
    // Write to PCA9536
    Wire.beginTransmission(I2C_PCA9536_Address);
    Wire.write(PCA9536_OUTPUT_PORT);
    Wire.write(controlByte);
    Wire.endTransmission();
  } else {
    Serial.print("[pcaPortOn] Error : ");
    Serial.println(result);
    Serial.println("[pcaPortOn] PCA9536 not found, check Vext");
  }

  Wire.end();
}

void pcaPortOff(uint8_t port) {
  Serial.print("[pcaPortOff] Switching output port ");
  Serial.print(port); 
  Serial.println(" OFF");

  byte result;
  Wire.begin();
  
  // Configure PCA9536 as outputs
  Wire.beginTransmission(I2C_PCA9536_Address);
  Wire.write(PCA9536_CONFIG);
  Wire.write(0x00); // 0x00 = all outputs
  result = Wire.endTransmission();

  if ( result == 0 ) {    
    // Set polarity inversion to normal
    Wire.beginTransmission(I2C_PCA9536_Address);
    Wire.write(PCA9536_POLARITY_INV);
    Wire.write(0x00); // 0x00 = normal polarity
    Wire.endTransmission();

    byte controlByte = 1 << port;
  
    // Write to PCA9536
    Wire.beginTransmission(I2C_PCA9536_Address);
    Wire.write(PCA9536_OUTPUT_PORT);
    Wire.write(controlByte ^ 0xFF); // Invert to set pin LOW
    Wire.endTransmission();
  } else {
    Serial.print("[pcaPortOff] Error : ");
    Serial.println(result);
    Serial.println("[pcaPortOff] PCA9536 not found, check Vext");
  }

  Wire.end();
}

// NeoPixel

void neoPixel(neoPixelColour_t colour) {
  uint8_t red, green, blue;
  // RGB can be 0..255, but 255 is very bright
  switch ( colour ) {
    case NP_OFF: {
      red = 0;
      green = 0;
      blue = 0;
      break;
    }
    case NP_RED: {
      red = 32;
      green = 0;
      blue = 0;
      break;
    }
    case NP_GREEN: {
      red = 0;
      green = 32;
      blue = 0;
      break;
    }
    case NP_BLUE: {
      red = 0;
      green = 0;
      blue = 32;
      break;
    }
    default:
      break;
  }

	neo.begin();                                        // Initialise RGB strip object
	neo.clear();                                        // Set all pixel 'off'
	neo.setPixelColor(0, neo.Color(red, green, blue));  // The first parameter is the NeoPixel index, and we only have one
	neo.show();                                         // Send the updated pixel colors to the hardware.
}

void cycleNeoPixel () {
  if (vextOnFlag) {
    Serial.println("[cycleNeoPixel] RED...");
    neoPixel(NP_RED);     // NeoPixel [low intensity] RED
    delay(1000);
    Serial.println("[cycleNeoPixel] GREEN...");
    neoPixel(NP_GREEN);   // NeoPixel [low intensity] GREEN
    delay(1000);
    Serial.println("[cycleNeoPixel] BLUE...");
    neoPixel(NP_BLUE);    // NeoPixel [low intensity] BLUE
    delay(1000);
    neoPixel(NP_OFF);     // NeoPixel OFF
  } else {
    Serial.println("[cycleNeoPixel] Vext must be ON to run the NeoPixel test");
  }
}

// Relay Control & Status LEDs
  // Nothing special here

// I2C bus scan

void scanI2cBus(uint8_t deviceAddress) {
  byte address;
  byte result;
  if ( vextOnFlag ) {
    Serial.println("[scanI2cBus] Scanning I2C bus...");
    Wire.begin();
    if ( deviceAddress == 0 ) {
      for ( address = 1; address < 127; address++ ) { 
        Wire.beginTransmission(address);
        result = Wire.endTransmission(); 
        if (result == 0)  {
          Serial.print("[scanI2cBus] I2C device found at address 0x");
          if (address < 16) 
            Serial.print("0");
          Serial.println(address,HEX);
        }
      }
    } else {
      Wire.beginTransmission(deviceAddress);
      result = Wire.endTransmission(); 
      if (result == 0)  {
        Serial.print("[scanI2cBus] I2C device found at address 0x");
      } else {
        Serial.print("[scanI2cBus] No response from address 0x");
      }
      if (deviceAddress < 16) 
        Serial.print("0");
      Serial.println(deviceAddress,HEX);
    }
    Wire.end();
  } else {
    Serial.print("[scanI2cBus] Turn Vext ON first...");
    Serial.println();
  }
}

// SD Card reader

void getSdCardInfo() {
  // set up variables using the SD utility library functions:
  Sd2Card card;
  SdVolume volume;
  SdFile root;

  if (card.init(SPI_HALF_SPEED, SPI_SD_CS)) {
    Serial.println("[getSdCardInfo] SD Card found");

    // print the type of card
    Serial.println();
    Serial.print("    Card type:         ");
    switch (card.type()) {
      case SD_CARD_TYPE_SD1:
        Serial.println("SD1");
        break;
      case SD_CARD_TYPE_SD2:
        Serial.println("SD2");
        break;
      case SD_CARD_TYPE_SDHC:
        Serial.println("SDHC");
        break;
      default:
        Serial.println("Unknown");
    }

    // Now try to open the 'volume'/'partition' - it should be FAT16 or FAT32
    if (volume.init(card)) {
      Serial.print("    Clusters:          ");
      Serial.println(volume.clusterCount());
      Serial.print("    Blocks x Cluster:  ");
      Serial.println(volume.blocksPerCluster());

      Serial.print("    Total Blocks:      ");
      Serial.println(volume.blocksPerCluster() * volume.clusterCount());
      Serial.println();

      // print the type and size of the first FAT-type volume
      uint32_t volumesize;
      Serial.print("    Volume type is:    FAT");
      Serial.println(volume.fatType(), DEC);

      volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
      volumesize *= volume.clusterCount();       // we'll have a lot of clusters
      volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
      Serial.print("    Volume size (Kb):  ");
      Serial.println(volumesize);
      Serial.print("    Volume size (Mb):  ");
      volumesize /= 1024;
      Serial.println(volumesize);
      Serial.print("    Volume size (Gb):  ");
      Serial.println((float)volumesize / 1024.0);

      Serial.println("\n    Files found on the card (name, date and size in bytes): ");
      root.openRoot(volume);

      // list all files in the card with date and size
      root.ls(LS_R | LS_DATE | LS_SIZE);
    } else {
      Serial.println("[getSdCardInfo] Could not find FAT16/FAT32 partition");
      Serial.println("                Make sure you've formatted the card");
    }
  } else {
    Serial.println("[getSdCardInfo] Initialization failed, check Vext");
  }
}

void deleteSdCardFile() {
  SPI.begin();
  if (SD.begin(SPI_SD_CS)) {
    SD.remove("testData.txt");
    Serial.println("[deleteSdCardFile] testData.txt deleted");
  } else {
    Serial.println("[deleteSdCardFile] SD Card initialization failed, check Vext");
  }
  SPI.end();
}

void existsSdCardFile() {
  SPI.begin();
  if (SD.begin(SPI_SD_CS)) {
    if (SD.exists("testData.txt")) {
      Serial.println("[existsSdCardFile] testData.txt found");
    } else {
      Serial.println("[existsSdCardFile] testData.txt not found");
    }
  } else {
    Serial.println("[existsSdCardFile] SD Card initialization failed, check Vext");
  }
  SPI.end();
}

void readSdCardFile() {
  File dataFile;
  SPI.begin();
  if (SD.begin(SPI_SD_CS)) {
    Serial.println("[readSdCardFile] SD Card initialized");
    Serial.println("[readSdCardFile] Open testData.txt to read...");
    dataFile = SD.open("testData.txt");
    if (dataFile) {
      Serial.println("[readSdCardFile] testData.txt content :");
      while (dataFile.available()) {
        Serial.write(dataFile.read());
      }
      dataFile.close();
    } else {
      Serial.println("[readSdCardFile] Error opening file to read, try writing first");
    }
  } else {
    Serial.println("[readSdCardFile] SD Card initialization failed, check Vext");
  }
  SPI.end();
}

void writeSdCardFile() {
  File dataFile;
  SPI.begin();
  if (SD.begin(SPI_SD_CS)) {
    Serial.println("[writeSdCardFile] SD Card initialized");
    Serial.println("[writeSdCardFile] Open testData.txt to write...");
    dataFile = SD.open("testData.txt", FILE_WRITE);
    if (dataFile) {
      Serial.println("[writeSdCardFile] Writing '[writeSdCardFile] Hello World!'");
      dataFile.println("[writeSdCardFile] Hello World!");
      dataFile.close();
    } else {
      Serial.println("[writeSdCardFile] Error opening file to write");
    }
  } else {
    Serial.println("[writeSdCardFile] SD Card initialization failed, check Vext");
  }
  SPI.end();
}

// Software revision levels

void printSoftwareRevision() {  
  String sketchRev = String(sketchRevision.major) + "." + String(sketchRevision.minor) + "." + String(sketchRevision.minimus);
  Serial.println("[printSoftRev] Sketch   " + sketchRev);
  Serial.println("[printSoftRev] EEPROM   " + eeprom.softwareRevision());
  Serial.println("[printSoftRev] Packet   " + packet.softwareRevision(PACKET_HANDLER));
  Serial.println("[printSoftRev] Node     " + packet.softwareRevision(NODE_HANDLER));
  Serial.println();
}

//Vext

void vextOn () {
  digitalWrite(Vext,LOW);
  Serial.println("[vextOn] Vext ON");
  vextOnFlag = true;
}

void vextOff () {
  digitalWrite(Vext,HIGH);
  Serial.println("[vextOff] Vext OFF");
  vextOnFlag = false;
}

// TPL5010 watchdog timer

void setWatchdogTimer(uint16_t interval) {
  if ( interval > 0 ) {
    TimerSetValue(&watchdogFeeder,interval*1000);
    TimerStart(&watchdogFeeder);
    Serial.print("[setWatchdogTimer] Watchdog timer set to fire every ");
    Serial.print(interval);
    Serial.println(" seconds");
  } else {
    TimerStop(&watchdogFeeder);
    Serial.println("[setWatchdogTimer] Watchdog timer cancelled");
  }
}

void feedTheDog() {
  Serial.println("[feedTheDog] Feeding the watchdog...");
  digitalWrite(WDT_DONE, HIGH);
  digitalWrite(WDT_DONE, LOW);
  TimerStart(&watchdogFeeder);
}