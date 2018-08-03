/*
 * MS5607.h
 */

#ifndef MS56XX_H_
#define MS56XX_H_


#include <asf.h>
#include "SPI.h"

typedef enum {
	MS5607 = 1,
	MS5611 = 2
} SENSOR_TYPE;

typedef enum {
	OSR_4096,
	OSR_2048,
	OSR_1024,
	OSR_512,
	OSR_256
} OSR_Settings;

typedef struct MS56XX_Data
{
	int32_t pressure; //Pascals
	int32_t temperature; //Centi-degrees celsius
	uint8_t valid; //1 = sensor believes data to be valid (no guarantee that it actually is), 0 = anomalous (all 0s or 1s) measurements
} MS56XX_Data_t;

typedef struct MS56XX
{
	//For end user usage
	ioport_pin_t select_pin;
	SPI_t* spi;
	SENSOR_TYPE model;
	MS56XX_Data_t data;
	OSR_Settings osr;
	
	//For internal use only
	uint16_t SENSt1; //C1
	uint16_t OFFt1; //C2
	uint16_t TCS; //You can guess
	uint16_t TCO;
	uint16_t Tref;
	uint16_t TEMPSENS;
} MS56XX_t;

void calibratePressureSensor(MS56XX_t* sensor);
void readMS56XX(MS56XX_t* sensor);

MS56XX_t define_new_MS56XX(SENSOR_TYPE model, SPI_t* spi, ioport_pin_t select_pin, OSR_Settings osr);
MS56XX_t define_new_MS56XX_default_OSR(SENSOR_TYPE model, SPI_t* spi, ioport_pin_t select_pin);


#endif /* MS5607_H_ */