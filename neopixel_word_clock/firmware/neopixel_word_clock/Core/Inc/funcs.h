/*
 * funcs.h
 *
 *  Created on: Jul 9, 2020
 *      Author: frank
 */

#ifndef INC_FUNCS_H_
#define INC_FUNCS_H_

#include "main.h"

#define get_xy_idx(x__, y__) (((y__) * MATRIX_WIDTH) + (x__))
#define get_time(ttttt) do { HAL_RTC_GetTime(&hrtc, (ttttt), RTC_FORMAT_BIN); volatile uint32_t tmpdate = hrtc.Instance->DR; tmpdate = (volatile uint32_t)(*(&(tmpdate))); } while (0)

void SystemClock_Config_KeepRTC(void);

void print_info(void);
void clock_task(void);
void reset_buffers(void);
void show_time(RTC_TimeTypeDef* t, uint8_t fadeInStyle, uint8_t fadeOutStyle);
void round_time(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout);
char is_time_different(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev);
void check_preserved_rows(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev);
void pack_minute_word(uint8_t minute);
void pack_hour_word(uint8_t hour, uint8_t minute);
void words_to_letters(void);
void shuffle_letters(void);
void handle_buttons(RTC_TimeTypeDef* now, RTC_TimeTypeDef* prev);
char handle_lightlevels(void);
void debug_framebuffer(void);
void debug_word(uint8_t word_code);
void debug_wordbuffer(void);
void debug_frameshade(void);
uint8_t light_read(void);
void show_strip(int32_t dly);

void time_add_secs(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int);
void time_add_mins(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int);
void time_add_hours(RTC_TimeTypeDef* tin, RTC_TimeTypeDef* tout, int);

void hsv_to_rgbw(pix_t* inPtr, rgbw_t* outPtr);
void hsv_to_rgbw_buffer(pix_t* inPtr, rgbw_t* outPtr, uint8_t cnt);
char pixel_brighten_linear(pix_t* pix, int16_t x, char stopAt);

void set_shown_svw(int16_t s, int16_t v, int16_t w);
void set_all_svw(int16_t s, int16_t v, int16_t w);
void blank_all(uint8_t s);
void fade_in_all(int16_t brite_step, int16_t brite_limit, char stopAt);
void fade_out_all(int16_t dim);

void fade_in_word(int16_t brite_step, int16_t brite_limit, char stopAt, uint8_t word_code);
void fade_in_words_all(int16_t brite_step, int16_t brite_limit, char stopAt);
void fade_in_letter(int16_t brite_step, int16_t brite_limit, char stopAt, uint8_t letter);
void snow_in_letters(int16_t brite_step, int16_t brite_limit, char stopAt);
void fade_out_letter(int16_t brite_step, uint8_t letter);
void snow_out_letters(int16_t brite_step);
void wipe_out(int16_t brite_step, int rowDly, uint8_t mode);

#endif /* INC_FUNCS_H_ */
