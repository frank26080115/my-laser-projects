/*

  WS2812B CPU and memory efficient library

  Date: 28.9.2016

  Author: Martin Hubacek
          http://www.martinhubacek.cz
          @hubmartin

  Modified by Frank Zhao https://www.eleccelerator.com/
           for the NeoPixel Word Clock project
           improvements have been submitted as pull-requests to the original git repo

  License: MIT License

*/

#ifndef WS2812B_H_
#define WS2812B_H_

#include <stdint.h>

#define WS2812B_HAS_WHITE
//#define WS2812B_USE_GAMMA_CORRECTION

// GPIO clock peripheral enable command
#define WS2812B_GPIO_CLK_ENABLE() __HAL_RCC_GPIOA_CLK_ENABLE()
// LED output port
#define WS2812B_PORT GPIOA
// LED output pins
#define WS2812B_PINS (GPIO_PIN_2)
// Timer 2 Channel
#define WS2812B_TIM_CHAN (TIM_CHANNEL_3)
// How many LEDs are in the series
#define WS2812B_NUMBER_OF_LEDS (8 * 8)
// Number of parallel LED strips on the SAME gpio. Each has its own buffer.
#define WS2812_BUFFER_COUNT 1

// Choose one of the bit-juggling setpixel implementation
// *******************************************************
//#define SETPIX_1	// For loop, works everywhere, slow
//#define SETPIX_2	// Optimized unrolled loop
#define SETPIX_3	// Unrolled loop plus some pointer increment optimization
//#define SETPIX_4


// DEBUG OUTPUT
// ********************
//#define LED4_PORT GPIOA
//#define LED4_PIN GPIO_PIN_1

//#define LED5_PORT GPIOA
//#define LED5_PIN GPIO_PIN_1


// Public functions
// ****************
void ws2812b_init();
void ws2812b_handle();

// Library structures
// ******************
// This value sets number of periods to generate 50uS Treset signal
#define WS2812_RESET_PERIOD 100

typedef struct ws2812b_buffer_item_t_ {
	uint8_t* frame_buffer_pointer;
	uint32_t frame_buffer_size;
	uint32_t frame_buffer_counter;
	uint8_t channel;	// digital output pin/channel
} ws2812b_buffer_item_t;

typedef struct ws2812b_t_
{
	ws2812b_buffer_item_t item[WS2812_BUFFER_COUNT];
	volatile uint8_t transfer_complete;
	uint8_t start_transfer;
	uint32_t timer_period_counter;
	uint32_t repeat_counter;
} ws2812b_t;

extern ws2812b_t ws2812b;

#endif /* WS2812B_H_ */
