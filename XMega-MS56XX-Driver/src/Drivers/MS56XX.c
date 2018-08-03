/*
 * MS5607.c
 *
 * Created: 9/25/2015 8:25:16 PM
 *  Author: dcorey
 */


 /*
 *	MS5611 info
 *
 *	Five commands: 1) Reset, 2) Read 128 bit PROM, 3) D1 conversion, 4) D2 conversion, 5) Read ADC result (24 bit temp/pressure)
 *	6 coefficients in PROM (all uint16_t)
 *		C1. Pressure sensitivity (SENSt1)
 *		C2. Pressure offset (OFFt1)
 *		C3. Temperature coefficient of pressure sensitivity (TCS)
 *		C4. Temperature coefficient of pressure offset (TCO)
 *		C5. Reference temperature (Tref)
 *		C6. Temperature coefficient of the temperature (TEMPSENS)
 *	D1 - pressure (digital)
 *	D2 - temperature (digital)
 *
 *	int32_t dt = D2 - C5 * 2^8
 *	int32_t TEMP = 2000 + dT * C6 / 2^23
 *
 *	int64_t OFF = C2 * 2^16 + (C4 * dT) / 2^7
 *	int64_t SENS = C1 * 2^15 + (C3 * dT) / 2^8
 *	int32_t PRESSURE = (D1 * SENS / 2^21 - OFF) / 2^15
 *	P (in millibars) = PRESSURE / 100.0
 *	PRESSURE is in Pascals.
 *
 *	If TEMP is less than 2000 (20 degrees Celsius), then
 *		uint32_t T2 = dT^2 / 2^31
 *		uint32_t OFF2 = 5 * (TEMP - 2000)^2 / 2
 *		uint32_t SENS2 = 5 * (TEMP - 2000)^2 / 4
 *		TEMP -= T2
 *		OFF -= OFF2
 *		SENS -= SENS2
 *
 */

#include "MS56XX.h"
#include <inttypes.h>
#include <asf.h>


void pressureSensorReset(MS56XX_t* sensor);
uint8_t get_read_info(OSR_Settings osr, uint8_t* D1_read_cmd, uint8_t* D2_read_cmd, uint16_t* delay_time_us);
uint16_t read16(SPI_t* targetspi);
uint32_t read24(SPI_t* targetspi);



uint16_t read16(SPI_t* targetspi)
{
	//Read in 16 bits, MSB first
	uint16_t ret = ((uint16_t)spiread(targetspi)) << 8;
	ret |= (uint16_t)spiread(targetspi);
	return ret;
}

uint32_t read24(SPI_t* targetspi)
{
	//Read in 24 bits, MSB first
	uint32_t ret = ((uint32_t)spiread(targetspi)) << 16;
	ret |= ((uint32_t)spiread(targetspi)) << 8;
	ret |= (uint32_t)spiread(targetspi);
	return ret;
}

MS56XX_t define_new_MS56XX(SENSOR_TYPE model, SPI_t* spi, ioport_pin_t select_pin, OSR_Settings osr)
{
	MS56XX_t pressure_sensor;
	pressure_sensor.model = model;
	pressure_sensor.select_pin = select_pin;
	pressure_sensor.spi = spi;
	pressure_sensor.osr = osr;
	return pressure_sensor;
}

MS56XX_t define_new_MS56XX_default_OSR(SENSOR_TYPE model, SPI_t* spi, ioport_pin_t select_pin)
{
	return define_new_MS56XX(model, spi, select_pin, OSR_4096); //Default to highest oversampling rate if not provided
}

void pressureSensorReset(MS56XX_t* sensor)
{
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b00011110);
	delay_ms(5);
	spideselect(sensor->select_pin);
	delay_ms(1);
}

void calibratePressureSensor(MS56XX_t* sensor)
//Set up the appropriate SPI before calling this
{
	pressureSensorReset(sensor);
	
	//Get all the lovely little calibration constants
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10100010); //Bits 1 - 3 are 001, for C1
	sensor->SENSt1 = read16(sensor->spi);
	spideselect(sensor->select_pin);

	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10100100); //010 = 2, for C2
	sensor->OFFt1 = read16(sensor->spi);
	spideselect(sensor->select_pin);

	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10100110); // 011 = 3, for C3
	sensor->TCS = read16(sensor->spi);
	spideselect(sensor->select_pin);

	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10101000); // 100 = 4
	sensor->TCO = read16(sensor->spi);
	spideselect(sensor->select_pin);

	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10101010); // 101 = 5
	sensor->Tref = read16(sensor->spi);
	spideselect(sensor->select_pin);


	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0b10101100); // 110 = 6
	sensor->TEMPSENS = read16(sensor->spi);
	spideselect(sensor->select_pin);

	/*printf("C1\tC2\tC3\tC4\tC5\tC6\t\n");
	printf("%u\t%u\t%u\t%u\t%u\t%u\n", 
			sensor->SENSt1, 
			sensor->OFFt1, 
			sensor->TCS, 
			sensor->TCO, 
			sensor->Tref, 
			sensor->TEMPSENS);*/
}


void readMS56XX(MS56XX_t* sensor)
 {
	uint32_t rawPressure = 0; //D1
	uint32_t rawTemp = 0; //D2
	
	//Assume data is valid unless any of the cases checked for are met
	sensor->data.valid = 1;
	
	uint16_t delay_time;
	uint8_t D1_cmd, D2_cmd;
	if (get_read_info(sensor->osr, &D1_cmd, &D2_cmd, &delay_time)) //Return flag of 1 = OSR not supported
	{
		//Mark data as invalid and exit function
		sensor->data.valid = 0;
		return;
	}
	//If get_read_info succeeded, D1_cmd, D2_cmd, and delay_time will now have the appropriate values for the selected OSR

	//Ask for raw pressure, 4096 OSR
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, D1_cmd);
	spideselect(sensor->select_pin);

	delay_us(delay_time);

	//Read off raw pressure (D1)
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0x0);
	rawPressure = read24(sensor->spi);
	spideselect(sensor->select_pin);
	
	//Ask for raw temperature, 4096 OSR
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, D2_cmd); //OSR = 4096
	spideselect(sensor->select_pin);
	
	delay_us(delay_time);

	//Read off raw temperature (D2)
	spiselect(sensor->select_pin);
	spiwrite(sensor->spi, 0x0);
	rawTemp = read24(sensor->spi);
	spideselect(sensor->select_pin);
	
	int32_t dT = rawTemp - (int32_t)(((int64_t)sensor->Tref) << 8);
	int32_t TEMP = (int32_t)(((int32_t)2000) + ((int32_t)(((int64_t)dT) * ((int64_t)sensor->TEMPSENS) >> 23)));
	
	int32_t T2 = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;
	
	if (TEMP < 2000)
	{
		T2 = ((int64_t)dT) * ((int64_t)dT) / ((int64_t)2147483648);
		OFF2 = ((int64_t)61) * ((int64_t)(TEMP - 2000)) * ((int64_t)(TEMP - 2000)) >> 4;
		SENS2 = ((int64_t)2) * ((int64_t)(TEMP - 2000)) * ((int64_t)(TEMP - 2000));
	}
	else
	{
	    T2 = 0;
		OFF2 = 0;
		SENS2 = 0;	
	} 
	
	if (TEMP<-1500)
	{
		OFF2 += ((int64_t)15) * (((int64_t) TEMP) + ((int64_t) 1500))^2; 
		SENS2 += ((int64_t) 8) * (((int64_t) TEMP) + ((int64_t) 1500) )^2;
	}
	uint8_t offshift1, offshift2, sens_shift1, sens_shift2;
	switch (sensor->model)
	{
		case MS5607:
			offshift1 = 17;
			offshift2 = 6;
			sens_shift1 = 16;
			sens_shift2 = 7;
			break;
		case MS5611:
			offshift1 = 17;
			offshift2 = 6;
			sens_shift1 = 16;
			sens_shift2 = 7;
			break;
		default:
			sensor->data.valid = 0;
			return;
	}
	int64_t OFF = (((int64_t)sensor->OFFt1) << offshift1) +
	((((int64_t)sensor->TCO) * ((int64_t)dT)) >> offshift2);
	
	int64_t SENS = (((int64_t)sensor->SENSt1) << sens_shift1) + ((((int64_t)sensor->TCS) * ((int64_t)dT)) >> sens_shift2);
		
	TEMP -= T2;
	OFF -= OFF2;
	SENS -= SENS2;
	
	int64_t PRESSURE = ((((int64_t)rawPressure) * SENS >> 21) - OFF) >> 15;
			
	/*printf("C1\tC2\tC3\tC4\tC5\tC6\t\n");
	printf("%u\t%u\t%u\t%u\t%u\t%u\n",
	pressureSensorCalibration.SENSt1,
	pressureSensorCalibration.OFFt1,
	pressureSensorCalibration.TCS,
	pressureSensorCalibration.TCO,
	pressureSensorCalibration.Tref,
	pressureSensorCalibration.TEMPSENS);*/
				
	/*printf("TEMP: %" PRIi32 "\n", TEMP);
	printf("Pressure: %" PRIi32 "\n", (int32_t)PRESSURE);*/
	
	sensor->data.pressure = (int32_t) PRESSURE; //In pascals
	sensor->data.temperature = TEMP; //In hundredths of degree celsius
 }
 
 uint8_t get_read_info(OSR_Settings osr, uint8_t* D1_read_cmd, uint8_t* D2_read_cmd, uint16_t* delay_time_us)
 {
	 switch (osr)
	 {
		 case OSR_4096:
			*D1_read_cmd = 0x48;
			*D2_read_cmd = 0x58;
			*delay_time_us = 9040;
			break;
		case OSR_2048:
			*D1_read_cmd = 0x46;
			*D2_read_cmd = 0x56;
			*delay_time_us = 4540;
			break;
		case OSR_1024:
			*D1_read_cmd = 0x44;
			*D2_read_cmd = 0x54;
			*delay_time_us = 2280;
			break;
		case OSR_512:
			*D1_read_cmd = 0x42;
			*D2_read_cmd = 0x52;
			*delay_time_us = 1170;
			break;
		case OSR_256:
			*D1_read_cmd = 0x40;
			*D2_read_cmd = 0x50;
			*delay_time_us = 600;
			break;
		default:
			return 1; //Error
	 }
	 return 0; //Success
 }
 
 