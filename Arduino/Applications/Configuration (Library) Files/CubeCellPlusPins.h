// Heltec CubeCell Dev-Board Plus pin identifiers

// CubeCell Dev-Board Plus predefined pin names
/*
		ADC
		ADC1		// ADC
		ADC2
		ADC3
	
		GPIO1
		GPIO2
		GPIO3
		GPIO4
		GPIO5
		GPIO6
		GPIO7
		GPIO8
		GPIO9
		GPIO10		// OLED Reset
		GPIO11
		GPIO12
		GPIO13
		GPIO14
		GPIO15		// Vext Control
		GPIO16		// USER_KEY, Vbat_ADC Control
	
		UART_RX		// Serial Chip
		UART_TX		// Serial Chip
		UART_RX1	// UART_RX
		UART_TX1	// UART_TX
		UART_RX2
		UART_TX2
		
		RGB			// GPIO13
		PWM1		// GPIO3
		PWM2		// GPIO4
	
		SCL			// OLED SCL
		SDA			// OLED SDA
		SCL1		// GPIO8
		SDA1		// GPIO9
	
		MOSI
		MISO
		SCK
		SS

		MOSI1		// GPIO1
		MISO1		// GPIO2
		SCK1		// GPIO3
		
		Vext 
		VBAT_ADC_CTL 
		USER_KEY
*/

#define ADC_CTL 					GPIO7

// I2C
// Nothing required here—SDA & SCL already defined

// ALF4all
#define A4a_A0						ADC3
#define A4a_A1						GPIO7
#define A4a_A2						GPIO9
#define A4a_A3						GPIO8

// Wind/Rain
#define rainCollectorPin	GPIO7
#define windDirectionPin	ADC3
#define windSpeedPin			GPIO8

// GPS
#define gpsTx							UART_RX2
#define gpsRx							UART_TX2

// AJ-SR04M Ultrasonic Distance Sensor
#define ultraTrig					UART_TX2
#define ultraEcho					UART_RX2

// Sensor Interface
#define sensorWake				GPIO5
#define sensorInterrupt		GPIO6

#define sphygOUT					GPIO5
#define sphygSCK					GPIO6

// Watchdog Timer
#define wdtDone						GPIO12

// Application Interrupt
#define interruptButton		GPIO11
