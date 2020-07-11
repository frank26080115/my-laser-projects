/*
 * vars.h
 *
 *  Created on: Jul 9, 2020
 *      Author: frank
 */

#ifndef INC_VARS_H_
#define INC_VARS_H_

#include "main.h"

extern rgbw_t rgbw_buffer[MATRIX_TOTAL];
extern pix_t  hsv_buffer[MATRIX_TOTAL];
extern uint8_t word_buffer[WORD_BUFF_LEN];
extern uint8_t word_buffer_idx;
extern uint8_t letter_buffer[LETTER_BUFF_LEN];
extern uint8_t letter_buffer_idx;
extern const uint8_t* word_list[];
extern uint8_t preserveRow;

extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc;
extern CRC_HandleTypeDef hcrc;
extern RTC_HandleTypeDef hrtc;

#if defined(USE_USB_PORT) && defined(USB) && defined(HAL_PCD_MODULE_ENABLED)
extern USBD_HandleTypeDef hUsbDeviceFS;
#endif

#endif /* INC_VARS_H_ */
