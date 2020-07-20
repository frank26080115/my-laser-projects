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
	static uint8_t prev = LIGHTLEVEL_NORMAL;
	static double filtered = -1;
	int x = adc_read(ADCCHAN_LIGHT);

	// implement a LPF
	double xd = x;
	if (filtered < 0) {
		filtered = xd;
	}
	filtered = (filtered * 0.9) + (xd * 0.1);

#ifdef DEBUG_LIGHTLEVEL
	static uint32_t dbgts = 0;
	if ((HAL_GetTick() - dbgts) > 500) {
		printf("light %lu\r\n", lround(filtered));
		dbgts = HAL_GetTick();
	}
#endif

#ifdef USE_LIGHT_SENSE_POTENTIOMETERS
	int pot1 = adc_read(ADCCHAN_POT_A);
	int pot2 = adc_read(ADCCHAN_POT_B);
#else
	// implement hysteresis
	if (prev == LIGHTLEVEL_NORMAL && filtered > 2400)
	{
		prev = LIGHTLEVEL_DARK;
	}
	else if (prev == LIGHTLEVEL_DARK && filtered < 2000)
	{
		prev = LIGHTLEVEL_NORMAL;
	}
	return prev;
#endif
#else
	return LIGHTLEVEL_NORMAL;
#endif
}
