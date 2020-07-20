/*
 * sleep.c
 *
 *  Created on: Jul 3, 2020
 *      Author: frank
 *
 *  This file contains the functions used for low power modes
 */

#include "main.h"

// goes into standby mode
void sleep(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Enable GPIOC clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

	hbled_off();

	/* Configure PA0 pin as input floating */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* Enable and set EXTI Interrupt to the lowest priority */
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	/*Clear all related wakeup flags*/
	LL_PWR_ClearFlag_WU();
	/*Re-enable all used wakeup sources: Pin1(PA.0)*/
	LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
	/*Enter the Standby mode*/
	HAL_PWR_EnterSTANDBYMode();
}

void power_init(void)
{
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Enable GPIOC clock */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

	/* Configure PA0 pin as input pull-down */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	LL_PWR_EnableBkUpAccess();

#ifndef STM32L052xx
	srand(adc_read(ADCCHAN_LIGHT));
#endif
}

void EXTI0_1_IRQHandler(void)
{
	// nothing to do
}
