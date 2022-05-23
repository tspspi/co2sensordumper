#define SERIAL_UART1_ENABLE 1
#define SERIAL_UART2_ENABLE 1
#define SERIAL_UART3_ENABLE 1
#define SENSOR_COUNT 3

#ifndef CO2DUMPINTERVAL_DEFAULT
	#define CO2DUMPINTERVAL_DEFAULT 10000000L
#endif

void co2ConcentrationReceived(
	unsigned char bChannel,
	uint16_t valueCO2,
	signed long int valueTemperature
);
