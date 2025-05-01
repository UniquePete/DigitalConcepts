// Heltec WiFi LoRa V1 pin identifiers

// Heltec WiFi LoRa V1 predefined pin names
/*
	static const uint8_t LED_BUILTIN = 25;
	#define BUILTIN_LED  LED_BUILTIN // backward compatibility
	#define LED_BUILTIN LED_BUILTIN

	static const uint8_t KEY_BUILTIN = 0;

	static const uint8_t TX			=  1;
	static const uint8_t RX			=  3;

	static const uint8_t SDA		= 21;
	static const uint8_t SCL		= 22;

	static const uint8_t SS			= 18;
	static const uint8_t MOSI		= 27;
	static const uint8_t MISO		= 19;
	static const uint8_t SCK		=  5;

	static const uint8_t A0			= 36;
	static const uint8_t A3			= 39;
	static const uint8_t A4			= 32;
	static const uint8_t A5			= 33;
	static const uint8_t A6			= 34;
	static const uint8_t A7			= 35;
	static const uint8_t A10		=  4;
	static const uint8_t A11		=  0;
	static const uint8_t A12		=  2;
	static const uint8_t A13		= 15;
	static const uint8_t A14		= 13;
	static const uint8_t A15		= 12;
	static const uint8_t A16		= 14;
	static const uint8_t A17		= 27;
	static const uint8_t A18		= 25;
	static const uint8_t A19		= 26;

	static const uint8_t T0			=  4;
	static const uint8_t T1			=  0;
	static const uint8_t T2			=  2;
	static const uint8_t T3			= 15;
	static const uint8_t T4			= 13;
	static const uint8_t T5			= 12;
	static const uint8_t T6			= 14;
	static const uint8_t T7			= 27;
	static const uint8_t T8			= 33;
	static const uint8_t T9			= 32;

	static const uint8_t DAC1		= 25;
	static const uint8_t DAC2		= 26;

	static const uint8_t Vext		= 21;
	static const uint8_t LED		= 25;
	static const uint8_t RST_OLED	= 16;
	static const uint8_t SCL_OLED	= 15;
	static const uint8_t SDA_OLED	=  4;
	static const uint8_t RST_LoRa	= 14;
	static const uint8_t DIO0		= 26;
	static const uint8_t DIO1		= 33;
	static const uint8_t DIO2		= 32;
*/

// General definitions
#define Builtin_LED LED_BUILTIN

#define GPIO0		 0
#define GPIO2		 2
#define GPIO4		 4
#define GPIO5		 5
#define GPIO12		12
#define GPIO13		13
#define GPIO14		14
#define GPIO15		15
#define GPIO16		16
#define GPIO17		17
#define GPIO18		18
#define GPIO19		19
#define GPIO21		21
#define GPIO22		22
#define GPIO23		23
#define GPIO25		25
#define GPIO26		26
#define GPIO27		27
#define GPIO32		32
#define GPIO33		33
#define GPIO34		34
#define GPIO35		35
#define GPIO36		36
#define GPIO37		37
#define GPIO38		38
#define GPIO39		39

#define VBAT_Read   13
#define	ADC_Ctrl    21

// I2C
// Must redefine SDA if using Vext/ADC_Ctrl (GPIO21)
#define	SDA      	GPIO21
#define	SCL      	GPIO22

// LoRa SPI
// Mostly already defined

#define CS			SS
#define RST			RST_LoRa

// ALF4all
#define A4a_A0		GPIO36
#define A4a_A1		GPIO13
#define A4a_A2		GPIO17
#define A4a_A3		GPIO23

// Wind/Rain
#define rainCollectorPin	GPIO35
#define windDirectionPin	GPIO37
#define windSpeedPin			GPIO34

// Flow Meter
#define flowMeterPin	GPIO37

// Function Interrupt
#define displayWake	GPIO0
#define triggerPin	GPIO22