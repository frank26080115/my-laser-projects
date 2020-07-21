/*
 * configs.h
 *
 *  Created on: Jul 7, 2020
 *      Author: frank
 */

#ifndef INC_CONFIGS_H_
#define INC_CONFIGS_H_

#include <stdint.h>

//#define USE_USB_PORT
#define USE_LIGHT_SENSE

#define MATRIX_HEIGHT  8
#define MATRIX_WIDTH   8
#define MATRIX_TOTAL   (MATRIX_HEIGHT * MATRIX_WIDTH)
#define FRAME_DELAY    (1000 / 25)

#define LED_DARK_V         0x04
#define LIGHT_DARK_THRESH  3400
#define LIGHT_DARK_HYSTER  50

#define ADCCHAN_LIGHT ADC_CHANNEL_3
#define ADCCHAN_POT_A ADC_CHANNEL_4
#define ADCCHAN_POT_B ADC_CHANNEL_5

#define btn_is_pressed_main()   (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_5) == 0 || LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_6) == 0)
#define btn_is_pressed_up()     (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_0) == 0)
#define btn_is_pressed_down()   (LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_1) == 0)
#define btn_is_pressed_any()    (btn_is_pressed_main() || btn_is_pressed_up() || btn_is_pressed_down())

#define hbled_on()              do { LL_GPIO_SetOutputPin  (GPIOA, LL_GPIO_PIN_7); } while (0)
#define hbled_off()             do { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7); } while (0)

#define power_avail()           (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_0) != 0)

#define WORD_BUFF_LEN    8
#define LETTER_BUFF_LEN 64

//#define DEBUG_FRAMEBUFFER
//#define DEBUG_FRAMESHADE
//#define DEBUG_WORDBUFFER
//#define DEBUG_PRESERVEDROW
//#define DEBUG_FADESTYLE
//#define DEBUG_LIGHTLEVEL

#endif /* INC_CONFIGS_H_ */
