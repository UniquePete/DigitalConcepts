// Heltec WiFi LoRa V3 pin identifiers

// Heltec WiFi LoRa V3 predefined pin names
/*
	static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT+48;
	#define BUILTIN_LED  LED_BUILTIN // backward compatibility
	#define LED_BUILTIN LED_BUILTIN
  #define RGB_BUILTIN LED_BUILTIN
  #define RGB_BRIGHTNESS 64

  #define analogInputToDigitalPin(p)  (((p)<20)?(analogChannelToDigitalPin(p)):-1)
  #define digitalPinToInterrupt(p)    (((p)<48)?(p):-1)
  #define digitalPinHasPWM(p)         (p < 46)

	static const uint8_t TX		= 43;
	static const uint8_t RX		= 44;

	static const uint8_t SDA = 41;
	static const uint8_t SCL = 42;

	static const uint8_t SS    = 8;
	static const uint8_t MOSI  = 10;
	static const uint8_t MISO  = 11;
	static const uint8_t SCK   = 9;

  static const uint8_t A0 = 1;
  static const uint8_t A1 = 2;
  static const uint8_t A2 = 3;
  static const uint8_t A3 = 4;
  static const uint8_t A4 = 5;
  static const uint8_t A5 = 6;
  static const uint8_t A6 = 7;
  static const uint8_t A7 = 8;
  static const uint8_t A8 = 9;
  static const uint8_t A9 = 10;
  static const uint8_t A10 = 11;
  static const uint8_t A11 = 12;
  static const uint8_t A12 = 13;
  static const uint8_t A13 = 14;
  static const uint8_t A14 = 15;
  static const uint8_t A15 = 16;
  static const uint8_t A16 = 17;
  static const uint8_t A17 = 18;
  static const uint8_t A18 = 19;
  static const uint8_t A19 = 20;

  static const uint8_t T1 = 1;
  static const uint8_t T2 = 2;
  static const uint8_t T3 = 3;
  static const uint8_t T4 = 4;
  static const uint8_t T5 = 5;
  static const uint8_t T6 = 6;
  static const uint8_t T7 = 7;
  static const uint8_t T8 = 8;
  static const uint8_t T9 = 9;
  static const uint8_t T10 = 10;
  static const uint8_t T11 = 11;
  static const uint8_t T12 = 12;
  static const uint8_t T13 = 13;
  static const uint8_t T14 = 14;

	static const uint8_t Vext		= 36;
	static const uint8_t LED		= 35;
	static const uint8_t RST_OLED	= 21;
	static const uint8_t SCL_OLED	= 18;
	static const uint8_t SDA_OLED	= 17;
	static const uint8_t RST_LoRa	= 12;
*/

//  static const uint8_t DIO0 = 26;
  
// General definitions
#define Builtin_LED 35

#define GPIO0		 0
#define GPIO1		 1
#define GPIO2		 2
#define GPIO3		 3
#define GPIO4		 4
#define GPIO5		 5
#define GPIO6		 6
#define GPIO7		 7

#define GPIO10		10
#define GPIO11		11
#define GPIO12		12
#define GPIO13		13
#define GPIO14		14

#define GPIO19		19
#define GPIO20		20
#define GPIO21		21

#define GPIO26		26

#define GPIO33		33
#define GPIO34		34
#define GPIO35		35
#define GPIO36		36
#define GPIO37		37
#define GPIO38		38
#define GPIO39		39
#define GPIO40		40
#define GPIO41		41
#define GPIO42		42
#define GPIO43		43
#define GPIO44		44
#define GPIO45		45
#define GPIO46		46
#define GPIO47		47
#define GPIO48		48

// Avoid GPIO0, GPIO1, GPIO3, GPIO45, GPIO46
// OK to use if you understand the consequences — these pins are strapped at boot

// General
#define USR_Button  GPIO0
#define VBAT_Read   GPIO1
#define LED_Write   GPIO35
#define	Vext_Ctrl   GPIO36
#define	ADC_Ctrl    GPIO37

#define BatM_Ctrl   GPIO6         // A4a_A1/sensorINT

// I2C
#define	SDA      	  GPIO19
#define	SCL      	  GPIO20

// LoRa SPI
#define CS			    GPIO8
#define NSS			    GPIO8
#define MOSI		    GPIO10
#define MISO		    GPIO11
#define SCK			    GPIO9
#define RST			    GPIO12
#define DIO0			  GPIO26
#define DIO1			  GPIO14

// Secondary SPI
#define CS			    GPIO4
#define MOSI			  GPIO5
#define MISO			  GPIO39
#define SCLK			  GPIO38

// ALF4all
#define A4a_A0		  GPIO2         // sensorWake
#define A4a_A1		  GPIO6         // BatM_Ctrl/sensorINT
#define A4a_A2		  GPIO33
#define A4a_A3		  GPIO40

// Wind/Rain
#define rainCollectorPin	GPIO34
#define windDirectionPin	GPIO47
#define windSpeedPin		  GPIO48

// Sensor Interface
#define sensorWake	GPIO2         // A4a_A0
#define sensorINT 	GPIO6         // BatM_Ctrl/A4a_A1

// Function Interrupt
#define displayWake	GPIO0
#define triggerPin	GPIO44
