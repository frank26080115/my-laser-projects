/*
 * lightsense.c
 *
 *  Created on: Jul 9, 2020
 *      Author: frank
 */
#include "main.h"

uint32_t light_read(void)
{
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
	return (uint32_t)lround(filtered);
}

// handles light sensor reading and brightness changes
// returns whether a change has been made
char handle_lightlevels(void)
{
	char changed = 0;
#if 1
	static uint32_t thresh = LIGHT_DARK_THRESH;
	uint32_t adc = light_read();
	uint8_t readLightLevel = prevLightLevel;

	// avoid rapid changes with hysteresis
	if (adc > thresh && prevLightLevel != LIGHTLEVEL_DARK) {
		readLightLevel = LIGHTLEVEL_DARK;
	}
	else if (adc < (thresh - LIGHT_DARK_HYSTER) && prevLightLevel == LIGHTLEVEL_DARK) {
		readLightLevel = LIGHTLEVEL_NORMAL;
	}

	// when in the dark, but transitioning to brighter light, or user presses button
	if ((prevLightLevel == LIGHTLEVEL_DARK || currLightLevel == LIGHTLEVEL_DARK) && (readLightLevel != LIGHTLEVEL_DARK || btn_is_pressed_main()))
	{
		printf("light level change, brighten\r\n");
		currLightLevel = LIGHTLEVEL_NORMAL;
		set_shown_hsvw(0, 0, 0xFF, readLightLevel == LIGHTLEVEL_BRIGHT ? 0xFF : 0x00);
		show_strip(0);
		changed = 1;
		if (btn_is_pressed_main())
		{
			uint32_t cnt = 0;
			while (btn_is_pressed_main())
			{
				// wait for hold beyond 1 second
				cnt++;
				HAL_Delay(1);
				if (cnt > 1000)
				{
					printf("light level reset thresh\r\n");
					thresh = LIGHT_DARK_THRESH; // set the threshold back to default
					break;
				}
			}
			// wait for release
			while (btn_is_pressed_main())
			{
				__NOP();
			}
		}
	}
	else if (currLightLevel != LIGHTLEVEL_DARK && readLightLevel == LIGHTLEVEL_DARK) {
		printf("light level change, darken\r\n");
		currLightLevel = LIGHTLEVEL_DARK;
		set_shown_hsvw(0, 0xFF, LED_DARK_V, 0x00);
		show_strip(0);
		changed = 1;
	}
	else {
		currLightLevel = readLightLevel;
		if (readLightLevel != LIGHTLEVEL_DARK && btn_is_pressed_main())
		{
			if (btn_is_pressed_main())
			{
				uint32_t cnt = 0;
				while (btn_is_pressed_main())
				{
					// wait for hold beyond 1 second
					cnt++;
					HAL_Delay(1);
					if (cnt > 1000)
					{
						// set the threshold to current levels, which is the new dark threshold
						thresh = adc;
						printf("light level set thresh, darken\r\n");
						currLightLevel = LIGHTLEVEL_DARK;
						set_shown_hsvw(0, 0xFF, LED_DARK_V, 0x00);
						show_strip(0);
						changed = 1;
						break;
					}
				}
				// wait for release
				while (btn_is_pressed_main())
				{
					__NOP();
				}
			}
		}
	}

	if (currLightLevel == LIGHTLEVEL_UNKNOWN) {
		currLightLevel = LIGHTLEVEL_NORMAL;
	}

	prevLightLevel = currLightLevel;
#else
	currLightLevel = LIGHTLEVEL_NORMAL;
#endif
	return changed;
}
