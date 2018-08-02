/*
 * SPI.h
 *
 * Created: 9/25/2015 5:24:59 PM
 *  Author: dcorey
 */

#ifndef SPI_H_
#define SPI_H_

#include <asf.h>

uint8_t spiread(SPI_t* targetspi);
void spiwrite(SPI_t* targetspi, uint8_t data);
void spiselect(ioport_pin_t pin);
void spideselect(ioport_pin_t pin);
void initializespi(SPI_t* targetspi, PORT_t* port);
void enable_select_pin(ioport_pin_t pin);

#endif /* SPI_H_ */