#include <asf.h>
#include <stdio.h>
#include "drivers/uart_tools.h"
#include "drivers/SPI.h"
#include "drivers/MS56XX.h"

#define COMMS_USART				USARTC0
#define USART_TX_PIN			IOPORT_CREATE_PIN(PORTC, 3)
#define USART_RX_PIN			IOPORT_CREATE_PIN(PORTC, 2)
#define PRESSURE_SELECT_PIN		IOPORT_CREATE_PIN(PORTC, 4)


//Example usage of MS5611/07 driver for One Monthers
int main (void)
{
	board_init();
	sysclk_init();

	UART_computer_init(&COMMS_USART, &PORTC, USART_TX_PIN, USART_RX_PIN);

	PORTE.DIR = 0xff;
	PORTE.OUT = 0x0f;
	
	MS56XX_t pressure_sensor = define_new_MS56XX_default_OSR(MS5607, &SPIC, PRESSURE_SELECT_PIN);
	
	initializespi(&SPIC, &PORTC);
	enable_select_pin(pressure_sensor.select_pin);
	
	//Pressure sensor initialization routine, also reads calibration data from sensor
	calibratePressureSensor(&pressure_sensor);
	
	readMS56XX(&pressure_sensor);

	printf("Pressure is %" PRIi32 ", temperature is %" PRIi32 "\n", pressure_sensor.data.pressure, pressure_sensor.data.temperature);
	
	while (1)
	{
		readMS56XX(&pressure_sensor);
		printf("Pressure is %" PRIi32 ", temperature is %" PRIi32 ", %s\n", pressure_sensor.data.pressure, pressure_sensor.data.temperature, pressure_sensor.data.valid ? "Valid" : "Not valid");
		delay_ms(1000);
	}
}
