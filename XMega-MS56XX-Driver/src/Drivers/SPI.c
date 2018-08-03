/*
 * SPI.c
 *
 * Created: 9/25/2015 5:24:26 PM
 *  Author: dcorey
 */

#include "SPI.h"

uint8_t spiread(SPI_t* targetspi)
{
	targetspi->DATA = 0xFE;
	while (!(targetspi->STATUS >> 7)); //Wait until data actually comes in
	return targetspi->DATA;

}

void spiwrite(SPI_t* targetspi, uint8_t data)
{
	targetspi->DATA = data;
	while (!(targetspi->STATUS >> 7)); //Wait for data to be sent
}

void spiselect(ioport_pin_t pin)
{
	ioport_set_pin_low(pin); //Setting low selects
}

void spideselect(ioport_pin_t pin)
{
	ioport_set_pin_high(pin); //Setting high deselects
}

void enable_select_pin(ioport_pin_t pin)
{
	ioport_set_pin_dir(pin, IOPORT_DIR_OUTPUT);
	spideselect(pin); //Ensure we default to the deselected state
}

void initializespi(SPI_t* targetspi, PORT_t* port)
{
	sysclk_enable_peripheral_clock(targetspi); //Enable peripheral clock for SPIC

	/*
	*	Initialize pins
	*		MOSI direction to out (Master out, slave in) (pin 5)
	*		MISO direction to in (Master in, slave out) (pin 6)
	*		SCK direction to out? (clock) (pin 7)
	*/
	port->DIRSET |= 0b10110000;  //4 (default SS), 5 (MOSI) & 7 (SCK) to out
	port->DIRSET &= 0b10111111; //6 (MISO) in
	port->OUT |= 0b10110000; //Setting 4 to high deselects it, in case it isn't the select we're using
	port->IN &= 0b10111111;

	/*
	*	Setup SPI CTRL register
	*		7 - Double clock speed (Yes)
	*		6 - Enable SPI module (No. Why on earth would we enable the module we're trying to use?)
	*		5 - 1: LSB first, 0: MSB first (0)
	*		4 - 1 for master, 0 for slave (Yes)
	*		3,2 - Transfer mode (00 or 11)
	*		1,0 - Clock prescaler (00, for 1/4, which gives us 1/2 because of bit 7)
	*/

	/*
	*	Max clock speed for MS5611 is 20 MHz, MCU clock = 32Hz
	*	Setting clock speed to 16 MHz
	*	Provides 24 bit pressure measurements
	*	MS5611 initialization requires reading data from 128 bit PROM on chip
	*	6 coefficients are stored in PROM, use them for converting data from D1 and D2 into accurate temperature and pressure
	*	Clock polarity and phase modes 0 and 3 are both acceptable
	*
	*/

	targetspi->CTRL = 0b11010000;
}