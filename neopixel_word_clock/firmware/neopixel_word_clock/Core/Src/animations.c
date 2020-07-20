/*
 * animations.c
 *
 *  Created on: Jul 8, 2020
 *      Author: frank
 *
 *  This file contains many animation algorithms for the word clock
 *  fade-in, fade-out, wipes, etc
 *
 *  Most of these functions allow you to set the rate of fading with "dim" or "brite_step"
 *  "brite_limit" is the upper limit of how bright a pixel could be
 *  "stopAt" can be used to prevent or allow the use of the W element in the LED
 *  For the wiping animations, "rowDly" defines a minimum time to linger on a row or column
 *  "mode" is the same as FADEOUT_****
 *
 *  Most of these functions will take into account any rows that should be preserved
 */

#include "main.h"
#include "ws2812b.h"

void fade_out_all(int16_t dim)
{
	uint8_t i, lim = get_xy_idx(0, preserveRow);
	uint32_t sum;
	do
	{
		if (btn_is_pressed_any()) {
			dim = 0x2FF; // pressing any button will instantly make everything black
		}

		sum = 0;
		for (i = 0; i < lim; i++)
		{
			sum += pixel_brighten_linear(&(hsv_buffer[i]), -dim, 0);
			// return of 1 means still more to go
		}
		show_strip(FRAME_DELAY);
	}
	while (sum != 0);
}

void fade_in_all(int16_t brite_step, int16_t brite_limit, char stopAt)
{
	uint8_t wi, li;
	uint32_t sum;
	char did = 0;
	do
	{
		sum = 0;

		if (btn_is_pressed_any()) {
			brite_step = 0x2FF; // pressing any button will finish the animation
		}

		if (brite_step > brite_limit) {
			brite_step = brite_limit;
		}
		brite_limit -= brite_step;

		for (wi = 0; wi < word_buffer_idx; wi++)
		{
			uint8_t word_code = word_buffer[wi];
			if ((preserveRow == 3 && (word_code == WORD_MINUTE_TO || word_code == WORD_MINUTE_PAST))
					|| (preserveRow == 4 && word_code >= WORD_HOUR_TWELVE)) {
				continue;
			}
			uint8_t* letters = (uint8_t*)(word_list[word_code]);
			for (li = 0; ;)
			{
				uint8_t letter = letters[li];
				li++;
				if (letter == 0xFF) { // end of word
					break;
				}
				//if (letter < get_xy_idx(0, preserveRow))
				{
					sum += pixel_brighten_linear(&(hsv_buffer[letter]), brite_step, stopAt);
					// return of 1 means still more to go
					did = 1;
				}
			}
		}
		if (did) {
			show_strip(FRAME_DELAY);
			did = 0;
		}
	}
	while (sum != 0 && brite_limit > 0);
}

void fade_in_word(int16_t brite_step, int16_t brite_limit, char stopAt, uint8_t word_code)
{
	uint8_t li;
	uint32_t sum;
	uint8_t* letters = (uint8_t*)(word_list[word_code]);

	do
	{
		sum = 0;

		if (btn_is_pressed_any()) {
			brite_step = 0x2FF; // pressing any button will finish the animation
		}

		if (brite_step > brite_limit) {
			brite_step = brite_limit;
		}
		brite_limit -= brite_step;

		for (li = 0; ;)
		{
			uint8_t letter = letters[li];
			li++;
			if (letter == 0xFF) { // end of word
				break;
			}
			sum += pixel_brighten_linear(&(hsv_buffer[letter]), brite_step, stopAt);
			// return of 1 means still more to go
		}

		show_strip(FRAME_DELAY);
	}
	while (sum != 0 && brite_limit > 0);
}

void fade_in_words_all(int16_t brite_step, int16_t brite_limit, char stopAt)
{
	uint8_t i;
	for (i = 0; i < word_buffer_idx; i++)
	{
		uint8_t word_code = word_buffer[i];
		if (preserveRow == 3 && (word_code == WORD_MINUTE_TO || word_code == WORD_MINUTE_PAST)) {
			continue;
		}
		if (preserveRow == 4 && word_code >= WORD_HOUR_TWELVE) {
			continue;
		}
		fade_in_word(brite_step, brite_limit, stopAt, word_code);
	}
}

void fade_in_letter(int16_t brite_step, int16_t brite_limit, char stopAt, uint8_t letter)
{
	uint32_t sum;

	do
	{
		sum = 0;

		if (btn_is_pressed_any()) {
			brite_step = 0x2FF; // pressing any button will finish the animation
		}

		if (brite_step > brite_limit) {
			brite_step = brite_limit;
		}
		brite_limit -= brite_step;

		if (letter < get_xy_idx(0, preserveRow)) {
			sum += pixel_brighten_linear(&(hsv_buffer[letter]), brite_step, stopAt);
			// return of 1 means still more to go
			show_strip(FRAME_DELAY);
		}
	}
	while (sum != 0 && brite_limit > 0);
}

void fade_in_letters(int16_t brite_step, int16_t brite_limit, char stopAt, char shuffle)
{
	uint8_t i;
	words_to_letters();
	if (shuffle) {
		shuffle_letters();
	}
	for (i = 0; i < letter_buffer_idx; i++)
	{
		uint8_t letter = letter_buffer[i];
		if (letter >= get_xy_idx(0, preserveRow)) {
			continue;
		}
		fade_in_letter(brite_step, brite_limit, stopAt, letter);
	}
}

void snow_in_letters(int16_t brite_step, int16_t brite_limit, char stopAt)
{
	fade_in_letters(brite_step, brite_limit, stopAt, 1);
}

void wipe_in_letters(int16_t brite_step, int16_t brite_limit, char stopAt)
{
	fade_in_letters(brite_step, brite_limit, stopAt, 0);
}

void fade_out_letter(int16_t brite_step, uint8_t letter)
{
	uint32_t sum;

	do
	{
		sum = 0;

		if (btn_is_pressed_any()) {
			brite_step = 0x2FF; // pressing any button will finish the animation
		}

		if (letter < get_xy_idx(0, preserveRow)) {
			sum += pixel_brighten_linear(&(hsv_buffer[letter]), -brite_step, 0);
			// return of 1 means still more to go
			show_strip(FRAME_DELAY);
		}
	}
	while (sum != 0);
}

void snow_out_letters(int16_t brite_step)
{
	uint8_t i;
	words_to_letters();
	shuffle_letters();
	for (i = 0; i < letter_buffer_idx; i++)
	{
		uint8_t letter = letter_buffer[i];
		if (letter >= get_xy_idx(0, preserveRow)) {
			continue;
		}
		fade_out_letter(brite_step, letter);
	}
}

// do a wipe-out animation
void wipe_out(int16_t brite_step, int rowDly, uint8_t mode)
{
	int x, y, i, sum;
	int ylim, xlim;
	int ts;
	char did = 0;

	switch (mode)
	{
	case FADEOUT_WIPELEFT:
	case FADEOUT_WIPERIGHT:
		xlim = MATRIX_HEIGHT;
		ylim = MATRIX_WIDTH;
		break;
	case FADEOUT_WIPEDOWN:
	default:
		ylim = MATRIX_HEIGHT;
		xlim = MATRIX_WIDTH;
		break;
	}

	for (y = 0; y < ylim; y++)
	{
		ts = HAL_GetTick();
		do
		{
			sum = 0;

			if (btn_is_pressed_any()) {
				brite_step = 0x2FF; // pressing any button will finish the animation
			}
			for (x = 0; x < xlim; x++)
			{
				switch (mode)
				{
				case FADEOUT_WIPELEFT:
					i = get_xy_idx((ylim - 1 - y), x);
					break;
				case FADEOUT_WIPERIGHT:
					i = get_xy_idx(y, x);
					break;
				case FADEOUT_WIPEDOWN:
				default:
					i = get_xy_idx(x, y);
					break;
				}

				if (i < get_xy_idx(0, preserveRow)) {
					sum += pixel_brighten_linear(&(hsv_buffer[i]), -brite_step, 0);
					// return of 1 means still more to go
					did = 1;
				}
			}
			if (did) {
				show_strip(FRAME_DELAY);
				did = 0;
			}
		}
		while (sum != 0);

		while ((HAL_GetTick() - ts) < rowDly) {
			__NOP();
		}
	}
}

void blank_all(uint8_t s)
{
	set_all_svw(s, 0, 0);
}

// set the S, V, and W for all pixels
// negative values means "preserve last setting"
// will ignore letters that are supposed to be preserved
void set_all_svw(int16_t s, int16_t v, int16_t w)
{
	uint8_t i, lim = get_xy_idx(0, preserveRow);
	for (i = 0; i < lim; i++)
	{
		hsv_buffer[i].s = (s >= 0) ? (uint8_t)s : hsv_buffer[i].s;
		hsv_buffer[i].v = (v >= 0) ? (uint8_t)v : hsv_buffer[i].v;
		hsv_buffer[i].w = (w >= 0) ? (uint8_t)w : hsv_buffer[i].w;
	}
}

// if a letter is active, it will take on the S, V, and W values given
// negative values means "preserve last setting"
void set_shown_hsvw(int16_t h, int16_t s, int16_t v, int16_t w)
{
	uint8_t wi, li;
	for (wi = 0; wi < word_buffer_idx; wi++)
	{
		uint8_t word_code = word_buffer[wi];
		uint8_t* letters = (uint8_t*)(word_list[word_code]);
		for (li = 0; ;)
		{
			uint8_t letter = letters[li];
			li++;
			if (letter == 0xFF) { // end of word
				break;
			}
			hsv_buffer[letter].h = (h >= 0) ? (uint8_t)h : hsv_buffer[letter].h;
			hsv_buffer[letter].s = (s >= 0) ? (uint8_t)s : hsv_buffer[letter].s;
			hsv_buffer[letter].v = (v >= 0) ? (uint8_t)v : hsv_buffer[letter].v;
			hsv_buffer[letter].w = (w >= 0) ? (uint8_t)w : hsv_buffer[letter].w;
		}
	}
}

