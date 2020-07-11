/*
 * defs.h
 *
 *  Created on: Jul 7, 2020
 *      Author: frank
 */

#ifndef INC_DEFS_H_
#define INC_DEFS_H_

#include "configs.h"
#include <stdint.h>

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t w;
}
rgbw_t;

typedef struct
{
	uint8_t h;
	uint8_t s;
	uint8_t v;
	uint8_t w;
}
pix_t;

enum
{
	WORD_NONE = 0,
	WORD_MINUTE_FIVE,       //  1
	WORD_MINUTE_TEN,        //  2
	WORD_MINUTE_QUARTER,    //  3
	WORD_MINUTE_TWENTY,     //  4
	WORD_MINUTE_TWENTYFIVE, //  5
	WORD_MINUTE_HALF,       //  6
	WORD_MINUTE_TO,         //  7
	WORD_MINUTE_PAST,       //  8
	WORD_HOUR_TWELVE,       //  9
	WORD_HOUR_ONE,          // 10
	WORD_HOUR_TWO,          // 11
	WORD_HOUR_THREE,        // 12
	WORD_HOUR_FOUR,         // 13
	WORD_HOUR_FIVE,         // 14
	WORD_HOUR_SIX,          // 15
	WORD_HOUR_SEVEN,        // 16
	WORD_HOUR_EIGHT,        // 17
	WORD_HOUR_NINE,         // 18
	WORD_HOUR_TEN,          // 19
	WORD_HOUR_ELEVEN,       // 20
	WORD_INVALID,           // 21
};

enum
{
	FADEIN_INSTANT,
	FADEIN_GLOBAL,
	FADEIN_BYWORD,
	FADEIN_SNOWLETTERS,
	FADEIN_RANDOM,
	FADEIN_INVALID,
};

enum
{
	FADEOUT_INSTANT,
	FADEOUT_INSTANT_2,
	FADEOUT_INSTANT_3,
	FADEOUT_GLOBAL,
	FADEOUT_GLOBAL_2,
	FADEOUT_GLOBAL_3,
	FADEOUT_SNOW,
	FADEOUT_SNOW_2,
	FADEOUT_SNOW_3,
	FADEOUT_WIPELEFT,
	FADEOUT_WIPERIGHT,
	FADEOUT_WIPEDOWN,
	FADEOUT_RANDOM,
	FADEOUT_INVALID,
};

enum
{
	STOPAT_NONE = 0,
	STOPAT_V = 1,
	STOPAT_S = 2,
	STOPAT_W = 2,
};

enum
{
	LIGHTLEVEL_UNKNOWN,
	LIGHTLEVEL_NORMAL,
	LIGHTLEVEL_BRIGHT,
	LIGHTLEVEL_DARK,
};

#endif /* INC_DEFS_H_ */
