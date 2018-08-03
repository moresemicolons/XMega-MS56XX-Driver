/*
 * uart_tools.c
 *
 * Created: 11/5/2015 10:52:03 PM
 *  Author: dcorey
 */ 

#include <asf.h>
#include "config/conf_usart_serial.h"
#include "drivers/uart_tools.h"

void UART_computer_init(USART_t* comms_usart, PORT_t* comms_port, ioport_pin_t tx_pin, ioport_pin_t rx_pin)
/* This sets up the UART pins that are used by the XBee (if plugged into a one month board), and by the computer during debugging
Call during startup.
Based on Adam's code template for one month, but with RX pin configuration. */
{
	static usart_serial_options_t options = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};
	
	gpio_configure_pin(tx_pin, IOPORT_DIR_OUTPUT);
	gpio_configure_pin(rx_pin, IOPORT_DIR_INPUT);
	sysclk_enable_peripheral_clock(comms_usart); 
	
	stdio_serial_init(comms_usart, &options);
}