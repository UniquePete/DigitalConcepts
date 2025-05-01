// Heltec CubeCell Dev-Board V2 pin identifiers

// CubeCell V2 predefined pin names
/*
		ADC
		ADC_VBAT

		GPIO0
		GPIO1
		PWM1					// GPIO2
		GPIO2
		PWM2					// GPIO3
		GPIO3
		RGB						// GPIO4
		GPIO4
		GPIO5
		Vext					// GPIO6
		GPIO6
		VBAT_ADC_CTL	// GPIO7
		USER_KEY			// GPIO7
		GPIO7

		UART_TX
		UART_RX
		UART_TX2			// Same as UART_TX
		UART_RX2			// Same as UART_RX

		SCL
		SDA
		SCL1					// Same as SCL
		SDA1					// Same as SDA

		MISO
		MOSI
		SCK
		SS
		MISO1					// Same as MISO
		MOSI1					// Same as MOSI
		SCK1					// Same as SCK
*/

// I2C
// Nothing required here, SDA & SCL already defined

// ALF4all
#define A4a_A0				ADC
#define A4a_A1				GPIO5
#define A4a_A2				GPIO1
#define A4a_A3				GPIO0

// Wind/Rain>
#define rainCollectorPin	GPIO5
#define windDirectionPin	ADC
#define windSpeedPin		GPIO0

// Sensor Interface
#define sensorWake			GPIO2
#define sensorInterrupt		GPIO3

#define sphygOUT			GPIO2
#define sphygSCK			GPIO3
