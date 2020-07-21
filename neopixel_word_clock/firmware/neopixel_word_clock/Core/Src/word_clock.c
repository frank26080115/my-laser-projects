/*
 * word_clock.c
 *
 *  Created on: Jul 3, 2020
 *      Author: frank
 *
 *  This file contains the high level word clock code
 *  user interface, some utility functions
 *  global variable declarations
 *
 *  The main.c file is mostly auto-generated so we want to avoid writing into it
 */

#include "main.h"
#include "ws2812b.h"

rgbw_t rgbw_buffer[MATRIX_TOTAL]; // RGBW frame buffer sent to the pixels
pix_t  hsv_buffer[MATRIX_TOTAL];  // HSV buffer used during editing

// which LEDs must be ON for each word
// 0xFF is the terminator
const uint8_t word_minute_five[]        = { 8+8+0, 8+8+1, 8+8+2, 8+8+3, 0xFF };
const uint8_t word_minute_ten[]         = { 1, 3, 4, 0xFF };
const uint8_t word_minute_quarter[]     = { 8+0, 8+1, 8+2, 8+3, 8+4, 8+5, 8+6, 0xFF };
const uint8_t word_minute_twenty[]      = { 1+0, 1+1, 1+2, 1+3, 1+4, 1+5, 0xFF };
const uint8_t word_minute_twentyfive[]  = { 1+0, 1+1, 1+2, 1+3, 1+4, 1+5, 8+8+0, 8+8+1, 8+8+2, 8+8+3, 0xFF };
const uint8_t word_minute_half[]        = { 20+0, 20+1, 20+2, 20+3, 0xFF };
const uint8_t word_minute_to[]          = { 8+8+8+4+0, 8+8+8+4+1, 0xFF };
const uint8_t word_minute_past[]        = { 8+8+8+1+0, 8+8+8+1+1, 8+8+8+1+2, 8+8+8+1+3, 0xFF };
const uint8_t word_hour_twelve[]        = { (8*6)+0, (8*6)+1, (8*6)+2, (8*6)+3, (8*6)+5, (8*6)+6, 0xFF };
const uint8_t word_hour_one[]           = { (8*7)+1, (8*7)+4, (8*7)+7, 0xFF };
const uint8_t word_hour_two[]           = { (8*6), (8*6)+1, (8*7)+1, 0xFF };
const uint8_t word_hour_three[]         = { (8*5)+3, (8*5)+4, (8*5)+5, (8*5)+6, (8*5)+7, 0xFF };
const uint8_t word_hour_four[]          = { (8*7)+0, (8*7)+1, (8*7)+2, (8*7)+3, 0xFF };
const uint8_t word_hour_five[]          = { (8*4)+0, (8*4)+1, (8*4)+2, (8*4)+3,  0xFF };
const uint8_t word_hour_six[]           = { (8*5)+0, (8*5)+1, (8*5)+2 , 0xFF };
const uint8_t word_hour_seven[]         = { (8*5)+0, (8*6)+2, (8*6)+5, (8*6)+6, (8*6)+7, 0xFF };
const uint8_t word_hour_eight[]         = { (8*4)+3, (8*4)+4, (8*4)+5, (8*4)+6, (8*4)+7, 0xFF };
const uint8_t word_hour_nine[]          = { (8*7)+4, (8*7)+5, (8*7)+6, (8*7)+7, 0xFF };
const uint8_t word_hour_ten[]           = { (8*4)+7, (8*5)+7, (8*6)+7, 0xFF };
const uint8_t word_hour_eleven[]        = { (8*6)+2, (8*6)+3, (8*6)+4, (8*6)+5, (8*6)+6, (8*6)+7, 0xFF };

// the order of letters on the display, used for debugging only
const char* letter_arrangement =
		"ATWENTYD"
		"QUARTERY"
		"FIVEHALF"
		"DPASTORO"
		"FIVEIGHT"
		"SIXTHREE"
		"TWELEVEN"
		"FOURNINE"
		;

// array of pointers, index matching the word enumeration
const uint8_t* word_list[] = {
		0,
		(uint8_t*)word_minute_five,
		(uint8_t*)word_minute_ten,
		(uint8_t*)word_minute_quarter,
		(uint8_t*)word_minute_twenty,
		(uint8_t*)word_minute_twentyfive,
		(uint8_t*)word_minute_half,
		(uint8_t*)word_minute_to,
		(uint8_t*)word_minute_past,
		(uint8_t*)word_hour_twelve,
		(uint8_t*)word_hour_one,
		(uint8_t*)word_hour_two,
		(uint8_t*)word_hour_three,
		(uint8_t*)word_hour_four,
		(uint8_t*)word_hour_five,
		(uint8_t*)word_hour_six,
		(uint8_t*)word_hour_seven,
		(uint8_t*)word_hour_eight,
		(uint8_t*)word_hour_nine,
		(uint8_t*)word_hour_ten,
		(uint8_t*)word_hour_eleven,
};

// what words are currently active
uint8_t word_buffer[WORD_BUFF_LEN];
uint8_t word_buffer_idx;

// a place to store all the letters currently shown
// used mainly for the snow effects
uint8_t letter_buffer[LETTER_BUFF_LEN];
uint8_t letter_buffer_idx;

// tracks light levels, used for transitioning between light levels
uint8_t prevLightLevel = LIGHTLEVEL_UNKNOWN;
uint8_t currLightLevel = LIGHTLEVEL_UNKNOWN;

// since the "past", "to", and hour are all on the lower rows, we can preserve them during transitions
uint8_t preserveRow = MATRIX_HEIGHT;

// this is the main task, run repeatedly from the main loop
void clock_task(void)
{
	static char notFirstRun = 0;
	static RTC_TimeTypeDef prev = {0}; // previous time used for comparison, difference used to trigger update of display
	RTC_TimeTypeDef now;

	get_time(&now);

	// run any additional application specific initialization here
	if (notFirstRun == 0) {
		reset_buffers();
	}

	handle_buttons(&now, &prev);
	char lightChanged = handle_lightlevels();

	// this comparison prevents the debug output from being flooded
	if (now.Seconds == prev.Seconds && notFirstRun != 0) {
		memcpy(&prev, &now, sizeof(RTC_TimeTypeDef));
		return;
	}

	if (currLightLevel != LIGHTLEVEL_DARK) {
		hbled_on();
	}

	printf("time %02u:%02u:%02u\r\n", now.Hours, now.Minutes, now.Seconds);

	// this comparison checks if the display actually needs to be updated
	if (is_time_different(&now, &prev) == 0 && notFirstRun != 0) {
		memcpy(&prev, &now, sizeof(RTC_TimeTypeDef));
		for (int i = 0; i < 100 && btn_is_pressed_any() == 0; i++) {
			HAL_Delay(1);
		}
		hbled_off();
		return;
	}

	if (lightChanged || notFirstRun == 0) {
		preserveRow = MATRIX_HEIGHT;
	}
	else {
		check_preserved_rows(&now, &prev);
	}

	// difference detected, do not let it be different on the next loop
	memcpy(&prev, &now, sizeof(RTC_TimeTypeDef));

	// update the display
	show_time(&now, FADEIN_RANDOM, FADEOUT_RANDOM);

	// no more initialization required
	notFirstRun = 1;

	hbled_off();
}

void reset_buffers(void)
{
	word_buffer_idx = 0;
	letter_buffer_idx = 0;
	//memset(word_buffer, 0xFF, WORD_BUFF_LEN);
	//memset(letter_buffer, 0xFF, LETTER_BUFF_LEN);
}

void show_time(RTC_TimeTypeDef* t, uint8_t fadeInStyle, uint8_t fadeOutStyle)
{
	int fadeStep, fadeLimit, stopAt, rowDly;

	RTC_TimeTypeDef rounded;
	round_time(t, &rounded); // round to nearest 5 minute mark

	printf("rounded time %02u:%02u\r\n", rounded.Hours, rounded.Minutes);

	if (fadeOutStyle == FADEOUT_RANDOM)
	{
		// when using randomization, do not allow instant fading mode, just to be interesting
		// also prevent the snow effect fade-out if it's the first ever fade-out
		do {
			fadeOutStyle = rand_read() % FADEOUT_RANDOM;
		}
		while (fadeOutStyle <= FADEOUT_INSTANT_3 || (word_buffer_idx <= 0 && fadeOutStyle >= FADEOUT_SNOW && fadeOutStyle <= FADEOUT_SNOW_3));
#ifdef DEBUG_FADESTYLE
		printf("fade out style %u\r\n", fadeOutStyle);
#endif
	}

	fadeStep = 0x200 / (1000 / FRAME_DELAY);
	rowDly = 200;

	switch (fadeOutStyle)
	{
	case FADEOUT_WIPEDOWN:
	case FADEOUT_WIPELEFT:
	case FADEOUT_WIPERIGHT:
		wipe_out(fadeStep, rowDly, fadeOutStyle);
		break;
	case FADEOUT_SNOW:
	case FADEOUT_SNOW_2:
	case FADEOUT_SNOW_3:
		snow_out_letters(fadeStep * 8);
		break;
	case FADEOUT_GLOBAL:
	case FADEOUT_GLOBAL_2:
	case FADEOUT_GLOBAL_3:
		fade_out_all(fadeStep);
		break;
	case FADEOUT_INSTANT:
	case FADEOUT_INSTANT_2:
	case FADEOUT_INSTANT_3:
	default:
		blank_all(currLightLevel == LIGHTLEVEL_DARK ? 0xFF : 0);
		show_strip(0);
		break;
	}

	// note: do not reset the buffers before the snow-out effect!
	reset_buffers();
	pack_minute_word(rounded.Minutes);
	pack_hour_word(rounded.Hours, rounded.Minutes);
#ifdef DEBUG_WORDBUFFER
	debug_wordbuffer();
#endif

	if (fadeInStyle == FADEIN_RANDOM) {
		// do not allow instant fade-in when using random mode
		fadeInStyle = rand_read() % (FADEIN_RANDOM - 1);
		fadeInStyle++;
#ifdef DEBUG_FADESTYLE
		printf("fade in style %u\r\n", fadeInStyle);
#endif
	}

	// choose appropriate fade in speeds and brightness levels based on detected light levels
	switch (currLightLevel)
	{
	case LIGHTLEVEL_BRIGHT:
		set_all_hsvw(-1, 0, -1, -1);
		fadeStep = 0x200 / (1000 / FRAME_DELAY);
		fadeLimit = 0x1FF;
		stopAt = STOPAT_W;
		break;
	case LIGHTLEVEL_DARK:
		set_all_hsvw(0, 0xFF, -1, -1);
		fadeStep = 0x80 / (1000 / FRAME_DELAY);
		fadeLimit = LED_DARK_V;
		stopAt = STOPAT_V;
		break;
	case LIGHTLEVEL_NORMAL:
	default:
		set_all_hsvw(-1, 0, -1, -1);
		fadeStep = 0x100 / (1000 / FRAME_DELAY);
		fadeLimit = 0xFF;
		stopAt = STOPAT_W;
		break;
	}

	switch (fadeInStyle)
	{
	case FADEIN_BYWORD:
		fade_in_words_all(fadeStep, fadeLimit, stopAt);
		break;
	case FADEIN_SNOWLETTERS:
		snow_in_letters(fadeStep * 8, fadeLimit, stopAt);
		break;
	case FADEIN_WIPELETTERS:
		wipe_in_letters(fadeStep * 4, fadeLimit, stopAt);
		break;
	case FADEIN_GLOBAL:
		fade_in_all(fadeStep, fadeLimit, stopAt);
		break;
	case FADEIN_INSTANT:
	default:
		fade_in_all(0x300, fadeLimit, stopAt);
		break;
	}

#ifdef DEBUG_FRAMEBUFFER
	debug_framebuffer();
#endif
}

// round the time to the nearest 5 minutes
void round_time(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout)
{
	int8_t rounded_minute = tin->Minutes;
	uint8_t rounded_hour = tin->Hours;

	if (tin->Seconds >= 30) {
		// round up the minute according to the second
		rounded_minute += 1;
	}
	if ((rounded_minute % 5) >=3) {
		// round up
		while ((rounded_minute % 5) != 0) {
			rounded_minute++;
			if (rounded_minute >= 60) {
				rounded_minute = 0;
				rounded_hour += 1;
			}
		}
	}
	else if ((rounded_minute % 5) < 3) {
		// round down
		while ((rounded_minute % 5) != 0) {
			rounded_minute--;
		}
	}

	if (rounded_minute >= 60) {
		rounded_minute = 0;
		rounded_hour += 1;
	}

	if (rounded_hour >= 12) {
		rounded_hour = 0;
	}
	tout->Minutes = rounded_minute;
	tout->Hours = rounded_hour;
}

// checks if the time being displayed should change
char is_time_different(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev)
{
	RTC_TimeTypeDef tmp1, tmp2;
	memcpy(&tmp1, now, sizeof(RTC_TimeTypeDef));
	memcpy(&tmp2, prev, sizeof(RTC_TimeTypeDef));
	round_time(now, &tmp1);
	round_time(prev, &tmp2);
	return (tmp1.Hours != tmp2.Hours || tmp1.Minutes != tmp2.Minutes);
}

// check if the previous display and current display have common words that can be preserved
void check_preserved_rows(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev)
{
	RTC_TimeTypeDef tmp1, tmp2;

	preserveRow = MATRIX_HEIGHT;

	memcpy(&tmp1, now, sizeof(RTC_TimeTypeDef));
	memcpy(&tmp2, prev, sizeof(RTC_TimeTypeDef));
	round_time(now, &tmp1);
	round_time(prev, &tmp2);

	// round up hours if using the word "to"
	if (tmp1.Minutes >= 35) {
		tmp1.Hours++;
	}
	tmp1.Hours %= 12;
	if (tmp2.Minutes >= 35) {
		tmp2.Hours++;
	}
	tmp2.Hours %= 12;

	// both uses the same hour word
	if (tmp1.Hours == tmp2.Hours)
	{
		preserveRow = 4;

		tmp1.Minutes /= 5;
		tmp2.Minutes /= 5;
		if ((tmp1.Minutes == 0 && tmp2.Minutes == 0) // both uses none
				|| (tmp1.Minutes >= 1 && tmp2.Minutes >= 1 && tmp1.Minutes <= 6 && tmp2.Minutes <= 6) // both uses "past"
				|| (tmp1.Minutes >= 7 && tmp2.Minutes >= 7) // both uses "to"
				)
		{
			preserveRow = 3;
		}
	}

#ifdef DEBUG_PRESERVEDROW
	if (preserveRow != MATRIX_HEIGHT) {
		printf("preserve row %u\r\n", preserveRow);
	}
#endif
}

// place the words about the minute into the word buffer
void pack_minute_word(uint8_t minute)
{
	minute /= 5;
	if (minute <= 0) {
		// if the minute is zero then just show the hour
		return;
	}
	if (minute == 5 || minute == 7) {
		// although we have "twenty five" defined as a separate entity, we still push in two words
		// this looks better when animated
		word_buffer[word_buffer_idx] = WORD_MINUTE_TWENTY;
		word_buffer_idx++;
		word_buffer[word_buffer_idx] = WORD_MINUTE_FIVE;
		word_buffer_idx++;
		word_buffer[word_buffer_idx] = (minute == 5) ? WORD_MINUTE_PAST : WORD_MINUTE_TO;
		word_buffer_idx++;
	}
	else if (minute >= 1 && minute <= 6) {
		// before or on the 30 minute mark
		word_buffer[word_buffer_idx] = WORD_NONE + minute;
		word_buffer_idx++;
		word_buffer[word_buffer_idx] = WORD_MINUTE_PAST;
		word_buffer_idx++;
	}
	else if (minute > 7) {
		// after the 35 minute mark
		word_buffer[word_buffer_idx] = WORD_NONE + (12 - minute);
		word_buffer_idx++;
		word_buffer[word_buffer_idx] = WORD_MINUTE_TO;
		word_buffer_idx++;
	}
}

// place the words about the hour into the word buffer
void pack_hour_word(uint8_t hour, uint8_t minute)
{
	if (minute >= 35) {
		hour++;
	}
	hour %= 12;
	word_buffer[word_buffer_idx] = WORD_HOUR_TWELVE + hour;
	word_buffer_idx++;
}

// unpack all words into the letters buffer
void words_to_letters(void)
{
	uint8_t i, j, w, letter;
	uint8_t* t;
	for (i = 0; i < word_buffer_idx; i++)
	{
		w = word_buffer[i];
		t = (uint8_t*)word_list[w];
		for (j = 0; j < 32; j++) {
			letter = t[j];
			if (letter == 0xFF) {
				break;
			}
			letter_buffer[letter_buffer_idx] = letter;
			letter_buffer_idx++;
		}
	}
}

// use random number generator to shuffle the letter ordering, for the random snow effect
void shuffle_letters(void)
{
	uint32_t prnd = rand_read(), rnd;
	uint8_t i;
	for (i = 0; i < letter_buffer_idx; i++)
	{
		while ((rnd = rand_read()) == prnd) {
			// wait for new random number
			__NOP();
		}
		while (((rnd = rand_read()) % letter_buffer_idx) == i) {
			// prevent swaps with self
			__NOP();
		}
		prnd = rnd; // next one must be new
		uint32_t ri = rnd % letter_buffer_idx; // keep in limit
		// do a swap
		uint8_t la = letter_buffer[i];
		uint8_t lb = letter_buffer[ri];
		letter_buffer[ri] = la;
		letter_buffer[i] = lb;
	}
}

// add seconds to the time while keeping everything valid
void time_add_secs(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int s)
{
	int i;
	memcpy(tout, tin, sizeof(RTC_TimeTypeDef));
	if (s >= 0)
	{
		for (i = 0; i < s; i++)
		{
			tout->Seconds++;
			if (tout->Seconds >= 60)
			{
				tout->Seconds = 0;
				tout->Minutes++;
				if (tout->Minutes >= 60)
				{
					tout->Minutes = 0;
					tout->Hours++;
					if (tout->Hours >= 12)
					{
						tout->Hours = 0;
					}
				}
			}
		}
	}
	else
	{
		s = -s;
		for (i = 0; i < s; i++)
		{
			if (tout->Seconds == 0)
			{
				tout->Seconds = 59;
				if (tout->Minutes == 0)
				{
					tout->Minutes = 59;
					if (tout->Hours == 0)
					{
						tout->Hours = 11;
					}
					else
					{
						tout->Hours--;
					}
				}
				else
				{
					tout->Minutes--;
				}
			}
			else
			{
				tout->Seconds--;
			}
		}
	}
}

void time_add_mins(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int m)
{
	int i, mm = m < 0 ? -m : m;
	for (i = 0; i < mm; i++) {
		time_add_secs(tin, tout, m < 0 ? -60 : 60);
	}
}

void time_add_hours(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int h)
{
	int i, hh = h < 0 ? -h : h;
	for (i = 0; i < hh; i++) {
		time_add_mins(tin, tout, h < 0 ? -60 : 60);
	}
}

void handle_buttons(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev)
{
	int pressHoldLimit = 800; // first hold is long, subsequent holds are shorter
	uint8_t loop = 0; // whether or not to loop
	do
	{
		uint8_t btnbits = ((btn_is_pressed_up() ? 1 : 0) << 1) | (btn_is_pressed_down() ? 1 : 0);
		loop = 0;
		RTC_TimeTypeDef tmp;

		// inc or dec depending on which button is pressed
		if (btnbits == 0x02) {
			time_add_mins(now, now, 5);
		}
		else if (btnbits == 0x01) {
			time_add_mins(now, now, -5);
		}
		else {
			// 0 or 3
			break;
		}

		hbled_on();

		// we only show the nearest 5 minutes so setting time can only set to the nearest 5 minutes
		round_time(now, &tmp);
		memcpy(now, &tmp, sizeof(RTC_TimeTypeDef));
		now->Seconds = 0;

		// without this, the RTC screws up
		now->TimeFormat = RTC_HOURFORMAT12_AM;
		now->DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		now->StoreOperation = RTC_STOREOPERATION_RESET;

		// set the time into the RTC
		HAL_RTC_SetTime(&hrtc, now, RTC_FORMAT_BIN);
		printf("new time %02u:%02u\r\n", now->Hours, now->Minutes);

		// show the time
		currLightLevel = LIGHTLEVEL_NORMAL;
		check_preserved_rows(now, prev);
		memcpy(prev, now, sizeof(RTC_TimeTypeDef));
		show_time(now, FADEIN_INSTANT, FADEOUT_INSTANT);

		HAL_Delay(50); // debounce
		uint32_t ts = HAL_GetTick();
		while ((btnbits == 0x02 && btn_is_pressed_up()) || (btnbits == 0x01 && btn_is_pressed_down())) // if still held down
		{
			if ((HAL_GetTick() - ts) > pressHoldLimit) // held down long enough
			{
				loop = 1; // do again

				// first hold is long, subsequent holds are shorter
				if (pressHoldLimit > 500) {
					pressHoldLimit = 400;
				}
				else if (pressHoldLimit >= 20){
					pressHoldLimit -= 20;
				}

				break;
			}
		}

		hbled_off();
	}
	while (loop != 0);
}

void debug_framebuffer(void)
{
	int8_t x, y, i;
	char tmpbuff[MATRIX_WIDTH + 2];
	char hdr;
	for (y = -1; y < MATRIX_HEIGHT + 1; y++)
	{
		for (x = 0; x < MATRIX_WIDTH; x++)
		{
			i = get_xy_idx(x, y);
			if (y < 0 || y >= MATRIX_HEIGHT) {
				hdr = '+';        // frame corner
				tmpbuff[x] = '-'; // frame upper and lower borders
			}
			else {
				uint32_t* pt = (uint32_t*)&rgbw_buffer[i];
				uint32_t p = *pt;
				if (p != 0) { // if not completely off
					tmpbuff[x] = letter_arrangement[i];
				}
				else {
					tmpbuff[x] = ' '; // blank
				}
				hdr = '|'; // frame side borders
			}
		}
		tmpbuff[x] = 0;
		printf("%c%s%c\r\n", hdr, tmpbuff, hdr);
	}
}

void debug_frameshade(void)
{
	static const char* shade_chars = " .:-=+*#%@"; // these characters represent different brightnesses
	int8_t x, y, i;
	char tmpbuff[MATRIX_WIDTH + 2];
	char hdr;
	for (y = -1; y < MATRIX_HEIGHT + 1; y++)
	{
		for (x = 0; x < MATRIX_WIDTH; x++)
		{
			i = get_xy_idx(x, y);
			if (y < 0 || y >= MATRIX_HEIGHT) {
				hdr = '+';        // frame corner
				tmpbuff[x] = '-'; // frame upper and lower borders
			}
			else
			{
				// sum up the RGBW values
				uint32_t psum = 0;
				psum += rgbw_buffer[i].r;
				psum += rgbw_buffer[i].g;
				psum += rgbw_buffer[i].b;
				psum += rgbw_buffer[i].w;

				psum += strlen(shade_chars) / 2; // rounding
				// figure out which shade character to use
				psum /= (255 * 4) / strlen(shade_chars);
				psum = psum > 9 ? 9 : psum;

				tmpbuff[x] = shade_chars[psum];

				hdr = '|'; // frame side borders
			}
		}
		tmpbuff[x] = 0;
		printf("%c%s%c\r\n", hdr, tmpbuff, hdr);
	}
}

void debug_word(uint8_t word_code)
{
	uint8_t li;
	uint8_t* letters = (uint8_t*)(word_list[word_code]);
	for (li = 0; ;)
	{
		uint8_t letter = letters[li];
		li++;
		if (letter == 0xFF) { // end of word
			printf(" ");
			break;
		}
		printf("%c", letter_arrangement[letter]);
	}
}

void debug_wordbuffer(void)
{
	uint8_t wi;
	printf("> ");
	for (wi = 0; wi < word_buffer_idx; wi++)
	{
		uint8_t word_code = word_buffer[wi];
		debug_word(word_code);
	}
	printf("<\r\n");
}
