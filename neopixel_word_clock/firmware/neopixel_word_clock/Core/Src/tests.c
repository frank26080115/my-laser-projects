/*
 * test.c
 *
 *  Created on: Jul 6, 2020
 *      Author: frank
 *
 *  This file contains hardware bring-up tests
 *  not required in final product
 */

#include "main.h"
#include "ws2812b.h"

void test_systick(void)
{
	while(1)
	{
		printf("systick %lu\r\n", HAL_GetTick());
		HAL_Delay(1000);
	}
}

void test_adc(void)
{
	while (1)
	{
		printf("light %d , pot A %d , pot B %d\r\n", adc_read(ADCCHAN_LIGHT), adc_read(ADCCHAN_POT_A), adc_read(ADCCHAN_POT_B));
		HAL_Delay(1000);
	}
}

void test_btns(void)
{
	uint8_t btns;
	uint8_t prev = 0;
	while (1)
	{
		btns = 0;
		btns |= btn_is_pressed_main() ? 0x4 : 0;
		btns |= btn_is_pressed_up()   ? 0x2 : 0;
		btns |= btn_is_pressed_down() ? 0x1 : 0;
		if (btns != prev) {
			printf("btns main %d , up %d , down %d\r\n"
					, (btns & 0x4) != 0 ? 1 : 0
					, (btns & 0x2) != 0 ? 1 : 0
					, (btns & 0x1) != 0 ? 1 : 0);
		}
		prev = btns;
	}
}

void show_strip(int32_t dly);

void test_leds(void)
{
	uint8_t i, j = 0, loop = 0;

	while (1)
	{
		for (i = 0; i < MATRIX_TOTAL; i++)
		{
			uint8_t x = j != i ? 0 : 0xFF;
			hsv_buffer[i].s = 0;
			hsv_buffer[i].v = x;
			hsv_buffer[i].w = x;
			rgbw_buffer[i].r = x;
			rgbw_buffer[i].g = x;
			rgbw_buffer[i].b = x;
			rgbw_buffer[i].w = x;
		}
		j++;
		j %= MATRIX_TOTAL;

		show_strip(200);

		if (j == 0) {
			loop++;
			printf("loop %u\r\n", loop);
		}
	}
}

void test_rtc(void)
{
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	printf("BKUP 0x%08lX\r\n", hrtc.Instance->BKP0R);

	while (1)
	{
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
		printf("time %02u:%02u:%02u\r\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
		HAL_Delay(1000);
	}
}

void test_pwr(void)
{
	RTC_TimeTypeDef sTime, sTime2 = {0};
	RTC_DateTypeDef sDate;

	while (1)
	{
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

		if (sTime2.Seconds != sTime.Seconds) {
			printf("time %02u:%02u:%02u\r\n", sTime.Hours, sTime.Minutes, sTime.Seconds);
			memcpy(&sTime2, &sTime, sizeof(RTC_TimeTypeDef));
		}

		if (power_avail() == 0) {
			printf("nap time\r\n");
			sleep();
		}
	}
}

void test_rng(void)
{
	uint32_t i = 0;
	while (1)
	{
		printf("RNG[%lu] 0x%08lX\r\n", i, LL_RNG_ReadRandData32(RNG));
		HAL_Delay(200);
		i++;
	}
}

void test_clock(void)
{
	RTC_TimeTypeDef now = {0};
	while (1)
	{
		show_time(&now, FADEIN_INSTANT, FADEOUT_INSTANT);
		time_add_mins(&now, &now, 5);
	}
}

void test_fade(void)
{
	static char notFirstRun = 0;
	RTC_TimeTypeDef now = {0};
	RTC_TimeTypeDef prev = {0};
	while (1)
	{
		if (notFirstRun == 0) {
			preserveRow = MATRIX_HEIGHT;
		}
		else {
			check_preserved_rows(&now, &prev);
		}
		show_time(&now, FADEIN_RANDOM, FADEOUT_RANDOM);
		memcpy(&prev, &now, sizeof(RTC_TimeTypeDef));
		time_add_mins(&now, &now, 5);
		notFirstRun = 1;
	}
}

void tests(void)
{
	//test_systick();
	//test_adc();
	//test_btns();
	//test_leds();
	//test_rtc();
	//test_pwr();
	//test_rng();
	//test_clock();
	//test_fade();
}
