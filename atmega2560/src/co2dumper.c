#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/twi.h>
#include <stdint.h>
#include <limits.h>

#include "./co2dumper.h"
#include "./serial.h"
#include "./sysclock.h"

static unsigned long int lastRequestMicro;

/*
	Simply send all values of all enabled sensors whenever a new value has been received ...
	This callback is called from serial.c
*/
struct sensorValues {
	uint16_t co2[SENSOR_COUNT];
	signed long int temperature[SENSOR_COUNT];
};
static struct sensorValues currentSensorValues;
static unsigned long int dwNewValues;

void co2ConcentrationReceived(
	unsigned char bChannel,
	uint16_t valueCO2,
	signed long int valueTemperature
) {
	currentSensorValues.co2[bChannel - 1] = valueCO2;
	currentSensorValues.temperature[bChannel - 1] = valueTemperature;

	dwNewValues = dwNewValues - 1;
	if(dwNewValues == 0) {
		/* Now send a new report ... */
		dwNewValues = SENSOR_COUNT;

		serialReportSensorData(
			currentSensorValues.co2,
			currentSensorValues.temperature,
			SENSOR_COUNT
		);
	}
}

int main() {
	#ifndef FRAMAC_SKIP
		cli();
	#endif

	/*
		Initialize state (power on reset)
	*/

	/*
		Initialize GPIO ports
	*/

	DDRB = DDRB | 0x80;

	/*
		If required later load configuration from EEPROM
	*/

	/* Clear debug LED */
	PORTB = PORTB & 0x7F;

	/* Setup system clock */
	sysclockInit();

		/*
		Setup serial
			USART0 is used to communicate via serial or USB interface
			USART1, USART2 and USART3 are used to communicate with sensors
	*/
	serialInit0();
	#ifdef SERIAL_UART1_ENABLE
		serialInit1();
	#endif
	#ifdef SERIAL_UART2_ENABLE
		serialInit2();
	#endif

	#ifndef FRAMAC_SKIP
		sei();
	#endif

	lastRequestMicro = micros();
	dwNewValues = SENSOR_COUNT;

	for(;;) {
		handleSerial0Messages();

		#ifdef SERIAL_UART1_ENABLE
			void handleSerial1Messages();
		#endif
		#ifdef SERIAL_UART2_ENABLE
			void handleSerial2Messages();
		#endif
		#ifdef SERIAL_UART3_ENABLE
			void handleSerial3Messages();
		#endif

		/* Blindly send request periodical. Remaining logic is in serial handlers */
		unsigned long int dwEllapsedSinceLastRequest;
		dwEllapsedSinceLastRequest = micros() - lastRequestMicro;
		if(dwEllapsedSinceLastRequest > CO2DUMPINTERVAL_DEFAULT) {
			lastRequestMicro = micros();
			serialRequestCO2Values(0);
		}
	}
}
