/*
 * lightsense.c
 *
 *  Created on: Jul 9, 2020
 *      Author: frank
 */
#include "main.h"

uint8_t light_read(void)
{
#ifdef USE_LIGHT_SENSE
	int x = adc_read(ADCCHAN_LIGHT);

#ifdef USE_LIGHT_SENSE_POTENTIOMETERS
	int pot1 = adc_read(ADCCHAN_POT_A);
	int pot2 = adc_read(ADCCHAN_POT_B);
#else

#endif
#endif
	return LIGHTLEVEL_NORMAL;
}
