/*
 * pixel.c
 *
 *  Created on: Jul 3, 2020
 *      Author: frank
 *
 *  This file is for pixel mathematics and pixel sending to the WS2812B library
 */

#include "main.h"
#include "ws2812b.h"

// increment or decrement the brightness of a single LED by a certain amount
// limit can be set with the "stopAt" param, can stop once V is maxed out, or S is maxed out, otherwise W will be used until it gets maxed out too
char pixel_brighten_linear(pix_t* pix, int16_t x, char stopAt)
{
	int16_t v = pix->v, s = pix->s, w = pix->w;
	if (x >= 0)
	{
		v += x;
		if (v >= 0xFF)
		{
			pix->v = 0xFF;
			if (stopAt == STOPAT_V) {
				return 0;
			}
			x = v - 0xFF;
			s -= x;
			if (s <= 0)
			{
				pix->s = 0;
				if (stopAt == STOPAT_S) {
					return 0;
				}
				x = -s;
				w += x;
				if (w >= 0xFF)
				{
					pix->w = 0xFF;
					return 0;
				}
				else
				{
					pix->w = w;
				}
			}
			else
			{
				pix->s = s;
			}
		}
		else
		{
			pix->v = v;
		}
	}
	else
	{
		x = -x;
		w -= x;
		if (w < 0)
		{
			pix->w = 0;
			x = -w;
			v -= x;
			if (v <= 0)
			{
				pix->v = 0;
				return 0;
			}
			else
			{
				pix->v = v;
			}
		}
		else
		{
			pix->w = w;
		}
	}
	return 1;
}

// convert all the HSV values from the buffer to RGB in the RGBW buffer
void hsv_to_rgbw_buffer(pix_t* inPtr, rgbw_t* outPtr, uint8_t cnt)
{
	uint8_t i;
	for (i = 0; i < cnt; i++)
	{
		pix_t* pixPtr = &(inPtr[i]);
		rgbw_t* rgbwPtr = &(outPtr[i]);
		hsv_to_rgbw(pixPtr, rgbwPtr);
	}
}

// convert HSV to RGB, copied from Adafruit's code
void hsv_to_rgbw(pix_t* inPtr, rgbw_t* outPtr)
{
	uint8_t r, g, b;
	uint16_t hue16 = inPtr->h;
	hue16 <<= 8;
	hue16 |= inPtr->h;
	hue16 = (hue16 * 1530L + 32768) / 65536;
	if (hue16 < 510) {                // Red to Green-1
		b = 0;
		if(hue16 < 255) {             // Red to Yellow-1
			r = 255;
			g = hue16;                // g = 0 to 254
		} else {                      // Yellow to Green-1
			r = 510 - hue16;          // r = 255 to 1
			g = 255;
		}
	} else if (hue16 < 1020) {        // Green to Blue-1
		r = 0;
		if(hue16 <  765) {            // Green to Cyan-1
				g = 255;
					b = hue16 - 510;  // b = 0 to 254
		} else {                      // Cyan to Blue-1
			g = 1020 - hue16;         // g = 255 to 1
			b = 255;
		}
	} else if (hue16 < 1530) {        // Blue to Red-1
		g = 0;
		if (hue16 < 1275) {           // Blue to Magenta-1
				r = hue16 - 1020;     // r = 0 to 254
					b = 255;
		} else {                      // Magenta to Red-1
			r = 255;
			b = 1530 - hue16;         // b = 255 to 1
		}
	} else {                          // Last 0.5 Red (quicker than % operator)
		r = 255;
		g = b = 0;
	}

	// Apply saturation and value to R,G,B, pack into 32-bit result:
	uint32_t v1 =   1 + inPtr->v; // 1 to 256; allows >>8 instead of /255
	uint16_t s1 =   1 + inPtr->s; // 1 to 256; same reason
	uint8_t  s2 = 255 - inPtr->s; // 255 to 0

	outPtr->r = ((((r * s1) >> 8) + s2) * v1) >> 8;
	outPtr->g = ((((g * s1) >> 8) + s2) * v1) >> 8;
	outPtr->b = ((((b * s1) >> 8) + s2) * v1) >> 8;
	outPtr->w = inPtr->w;
}

// show the entire frame buffer, at a set frame rate (set by a delay)
void show_strip(int32_t dly)
{
	static int32_t ts = 0; // timestamp to keep the frame rate consistent
	int32_t now;
	// wait until previous transfer is complete
	while (ws2812b.transfer_complete == 0) {
		__NOP();
	}
	// do conversion between buffer types
	hsv_to_rgbw_buffer(hsv_buffer, rgbw_buffer, MATRIX_TOTAL);

#ifdef DEBUG_FRAMESHADE
	debug_frameshade();
#endif

	// set the new frame
	// (this doesn't need to be here since it's always the same, but you won't notice the performance)
	ws2812b.item[0].frame_buffer_pointer = (uint8_t*)rgbw_buffer;
	ws2812b.item[0].frame_buffer_size = sizeof(rgbw_t) * MATRIX_TOTAL;
	ws2812b.item[0].frame_buffer_counter = 0;
	ws2812b.item[0].channel = 0;

	// wait until the next frame time
	now = HAL_GetTick();
	if (dly > 0)
	{
		while ((now = HAL_GetTick()) - dly < ts) {
			__NOP();
		}
	}
	ts = now;

	// fire up the DMA transfer to the LEDs
	ws2812b.start_transfer = 1;
	ws2812b_handle();
}
