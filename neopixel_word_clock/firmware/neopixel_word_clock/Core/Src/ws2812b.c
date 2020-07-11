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

#include <string.h>

#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_tim.h"
#include "ws2812b.h"

ws2812b_t ws2812b;

static uint8_t dma_bit_buffer[
#ifdef WS2812B_HAS_WHITE
							  32
#else
							  24
#endif
							  * 2];
#define BUFFER_SIZE		(sizeof(dma_bit_buffer)/sizeof(uint8_t))

static TIM_HandleTypeDef  timer2_handle;
static TIM_OC_InitTypeDef timer2_oc;

static uint32_t timer_period;
static uint8_t compare_pulse_logic_0;
static uint8_t compare_pulse_logic_1;


void dma_transfer_complete_handler(DMA_HandleTypeDef *dma_handle);
void dma_transfer_half_handler(DMA_HandleTypeDef *dma_handle);
static void ws2812b_set_pixel(uint8_t row, uint16_t column, uint8_t red, uint8_t green, uint8_t blue
#ifdef WS2812B_HAS_WHITE
		, uint8_t white
#endif
		);


#ifdef WS2812B_USE_GAMMA_CORRECTION
// Gamma correction table
const uint8_t gammaTable[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};
#endif

static void ws2812b_gpio_init(void)
{
	// WS2812B outputs
	WS2812B_GPIO_CLK_ENABLE();
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin       = WS2812B_PINS;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM2;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(WS2812B_PORT, &GPIO_InitStruct);

	// Enable output pins for debugging to see DMA Full and Half transfer interrupts
	#if defined(LED4_PORT) && defined(LED5_PORT)
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

		GPIO_InitStruct.Pin = LED4_PIN;
		HAL_GPIO_Init(LED4_PORT, &GPIO_InitStruct);
		GPIO_InitStruct.Pin = LED5_PIN;
		HAL_GPIO_Init(LED5_PORT, &GPIO_InitStruct);
	#endif
}

static void tim2_init(void)
{
	// TIM2 Periph clock enable
	__HAL_RCC_TIM2_CLK_ENABLE();

	// This computation of pulse length should work ok,
	// at some slower core speeds it needs some tuning.
	timer_period =  SystemCoreClock / 800000; // 0,125us period (10 times lower the 1,25us period to have fixed math below)

	uint32_t logic_0 = (10 * timer_period) / 36;
	uint32_t logic_1 = (10 * timer_period) / 15;

	if(logic_0 > 255 || logic_1 > 255)
	{
		// Error, compare_pulse_logic_0 or compare_pulse_logic_1 needs to be redefined to uint16_t (but it takes more memory)
		for(;;);
	}

	compare_pulse_logic_0 = logic_0;
	compare_pulse_logic_1 = logic_1;

	timer2_handle.Instance = TIM2;
	timer2_handle.Init.Period            = timer_period;
	timer2_handle.Init.Prescaler         = 0x00;
	timer2_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	timer2_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
	HAL_TIM_PWM_Init(&timer2_handle);

	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);

	timer2_oc.OCMode       = TIM_OCMODE_PWM1;
	timer2_oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
	timer2_oc.Pulse        = compare_pulse_logic_0;
	timer2_oc.OCFastMode   = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&timer2_handle, &timer2_oc, WS2812B_TIM_CHAN);

	HAL_TIM_PWM_Start(&timer2_handle, WS2812B_TIM_CHAN);

	(&timer2_handle)->Instance->DCR =
#if (WS2812B_TIM_CHAN == TIM_CHANNEL_1)
			TIM_DMABASE_CCR1
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_2)
			TIM_DMABASE_CCR2
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_3)
			TIM_DMABASE_CCR3
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_4)
			TIM_DMABASE_CCR4
#else
#error
#endif
			| TIM_DMABURSTLENGTH_1TRANSFER;

}


DMA_HandleTypeDef     dmaUpdate;


static void dma_init(void)
{

	// TIM2 Update event - Channel 2
	__HAL_RCC_DMA1_CLK_ENABLE();

	dmaUpdate.Init.Direction = DMA_MEMORY_TO_PERIPH;
	dmaUpdate.Init.PeriphInc = DMA_PINC_DISABLE;
	dmaUpdate.Init.MemInc = DMA_MINC_ENABLE;
	dmaUpdate.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	dmaUpdate.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
	dmaUpdate.Init.Mode = DMA_CIRCULAR;
	dmaUpdate.Init.Priority = DMA_PRIORITY_VERY_HIGH;
	dmaUpdate.Instance = DMA1_Channel2;
	dmaUpdate.Init.Request = DMA_REQUEST_8;

	dmaUpdate.XferCpltCallback  = dma_transfer_complete_handler;
	dmaUpdate.XferHalfCpltCallback = dma_transfer_half_handler;

    __HAL_LINKDMA(&timer2_handle, hdma[TIM_DMA_ID_UPDATE], dmaUpdate);

	HAL_DMA_Init(&dmaUpdate);


	HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
	//HAL_DMA_Start_IT(&dmaUpdate, (uint32_t)timDmaTest, (uint32_t)&(&Tim2Handle)->Instance->DMAR, 4);
	HAL_DMA_Start_IT(&dmaUpdate, (uint32_t)dma_bit_buffer, (uint32_t)&TIM2->
#if (WS2812B_TIM_CHAN == TIM_CHANNEL_1)
			CCR1
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_2)
			CCR2
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_3)
			CCR3
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_4)
			CCR4
#else
#error
#endif
			, BUFFER_SIZE);

}



void DMA1_Channel2_3_IRQHandler(void)
{
  // Check the interrupt and clear flag
  HAL_DMA_IRQHandler(&dmaUpdate);
}



static void load_next_framebuffer_data(ws2812b_buffer_item_t *buffer_item, uint32_t row)
{

	uint32_t r = buffer_item->frame_buffer_pointer[buffer_item->frame_buffer_counter++];
	uint32_t g = buffer_item->frame_buffer_pointer[buffer_item->frame_buffer_counter++];
	uint32_t b = buffer_item->frame_buffer_pointer[buffer_item->frame_buffer_counter++];
#ifdef WS2812B_HAS_WHITE
	uint32_t w = buffer_item->frame_buffer_pointer[buffer_item->frame_buffer_counter++];
#endif

	if(buffer_item->frame_buffer_counter == buffer_item->frame_buffer_size)
		buffer_item->frame_buffer_counter = 0;

	ws2812b_set_pixel(buffer_item->channel, row, r, g, b
#ifdef WS2812B_HAS_WHITE
			, w
#endif
			);
}


// Transmit the framebuffer
static void ws2812b_send()
{
	// transmission complete flag
	ws2812b.transfer_complete = 0;

	uint32_t i;

	for( i = 0; i < WS2812_BUFFER_COUNT; i++ )
	{
		ws2812b.item[i].frame_buffer_counter = 0;

		load_next_framebuffer_data(&ws2812b.item[i], 0);
		load_next_framebuffer_data(&ws2812b.item[i], 1);
	}

	HAL_TIM_Base_Stop(&timer2_handle);
	(&timer2_handle)->Instance->CR1 &= ~((0x1U << (0U)));


	// clear all DMA flags
	__HAL_DMA_CLEAR_FLAG(&dmaUpdate, DMA_FLAG_TC2 | DMA_FLAG_HT2 | DMA_FLAG_TE2);

	// configure the number of bytes to be transferred by the DMA controller
	dmaUpdate.Instance->CNDTR = BUFFER_SIZE;

	// clear all TIM2 flags
	__HAL_TIM_CLEAR_FLAG(&timer2_handle, TIM_FLAG_UPDATE | TIM_FLAG_CC1 | TIM_FLAG_CC2 | TIM_FLAG_CC3 | TIM_FLAG_CC4);

	// enable DMA channels
	__HAL_DMA_ENABLE(&dmaUpdate);

	// IMPORTANT: enable the TIM2 DMA requests AFTER enabling the DMA channels!
	__HAL_TIM_ENABLE_DMA(&timer2_handle, TIM_DMA_UPDATE);

	TIM2->CNT = timer_period - 1;

	// Set zero length for first pulse because the first bit loads after first TIM_UP
	TIM2->
#if (WS2812B_TIM_CHAN == TIM_CHANNEL_1)
			CCR1
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_2)
			CCR2
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_3)
			CCR3
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_4)
			CCR4
#else
#error
#endif
			= 0;

	// Enable PWM
#if (WS2812B_TIM_CHAN == TIM_CHANNEL_1)
	//(&timer2_handle)->Instance->CCMR1 &= ~(TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1);
	(&timer2_handle)->Instance->CCMR1 |= TIM_CCMR1_OC1M_1;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_2)
	//(&timer2_handle)->Instance->CCMR1 &= ~(TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1);
	(&timer2_handle)->Instance->CCMR1 |= TIM_CCMR1_OC2M_1;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_3)
	//(&timer2_handle)->Instance->CCMR2 &= ~(TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1);
	(&timer2_handle)->Instance->CCMR2 |= TIM_CCMR2_OC3M_1;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_4)
	//(&timer2_handle)->Instance->CCMR2 &= ~(TIM_CCMR2_OC4M_0 | TIM_CCMR2_OC4M_1);
	(&timer2_handle)->Instance->CCMR2 |= TIM_CCMR2_OC4M_1;
#else
#error
#endif

	__HAL_DBGMCU_FREEZE_TIM2();

	// start TIM2
	__HAL_TIM_ENABLE(&timer2_handle);
}





void dma_transfer_half_handler(DMA_HandleTypeDef *dma_handle)
{
	#if defined(LED4_PORT)
		LED4_PORT->BSRR = LED4_PIN;
	#endif

	// Is this the last LED?
	if(ws2812b.repeat_counter != (WS2812B_NUMBER_OF_LEDS / 2 - 1))
	{
		uint32_t i;

		for( i = 0; i < WS2812_BUFFER_COUNT; i++ )
		{
			load_next_framebuffer_data(&ws2812b.item[i], 0);
		}

	} else {

		//HAL_TIM_PWM_ConfigChannel()


		// If this is the last pixel, set the next pixel value to zeros, because
		// the DMA would not stop exactly at the last bit.
		ws2812b_set_pixel(0, 0, 0, 0, 0
#ifdef WS2812B_HAS_WHITE
				, 0
#endif
				);
	}

	#if defined(LED4_PORT)
		LED4_PORT->BRR = LED4_PIN;
	#endif
}

void dma_transfer_complete_handler(DMA_HandleTypeDef *dma_handle)
{
	#if defined(LED5_PORT)
		LED5_PORT->BSRR = LED5_PIN;
	#endif

	ws2812b.repeat_counter++;

	if(ws2812b.repeat_counter == WS2812B_NUMBER_OF_LEDS / 2)
	{
		// Transfer of all LEDs is done, disable DMA but enable tiemr update IRQ to stop the 50us pulse
		ws2812b.repeat_counter = 0;

		// Disable PWM output
#if (WS2812B_TIM_CHAN == TIM_CHANNEL_1)
		(&timer2_handle)->Instance->CCMR1 &= ~(TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1);
		(&timer2_handle)->Instance->CCMR1 |= TIM_CCMR1_OC1M_2;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_2)
		(&timer2_handle)->Instance->CCMR1 &= ~(TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1);
		(&timer2_handle)->Instance->CCMR1 |= TIM_CCMR1_OC2M_2;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_3)
		(&timer2_handle)->Instance->CCMR2 &= ~(TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1);
		(&timer2_handle)->Instance->CCMR2 |= TIM_CCMR2_OC3M_2;
#elif (WS2812B_TIM_CHAN == TIM_CHANNEL_4)
		(&timer2_handle)->Instance->CCMR2 &= ~(TIM_CCMR2_OC4M_0 | TIM_CCMR2_OC4M_1);
		(&timer2_handle)->Instance->CCMR2 |= TIM_CCMR2_OC4M_2;
#else
#error
#endif

		// Enable TIM2 Update interrupt for 50us Treset signal
		__HAL_TIM_ENABLE_IT(&timer2_handle, TIM_IT_UPDATE);
		// Disable DMA
		__HAL_DMA_DISABLE(&dmaUpdate);
		// Disable the DMA requests
		__HAL_TIM_DISABLE_DMA(&timer2_handle, TIM_DMA_UPDATE);

	} else {

		// Load bitbuffer with next RGB LED values
		uint32_t i;
		for( i = 0; i < WS2812_BUFFER_COUNT; i++ )
		{
			load_next_framebuffer_data(&ws2812b.item[i], 1);
		}

	}

	#if defined(LED5_PORT)
		LED5_PORT->BRR = LED5_PIN;
	#endif
}


void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&timer2_handle);
}

// TIM2 Interrupt Handler gets executed on every TIM2 Update if enabled
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

#if defined(LED5_PORT)
		LED5_PORT->BSRR = LED5_PIN;
	#endif

	// I have to wait 50us to generate Treset signal
	if (ws2812b.timer_period_counter < (uint8_t)WS2812_RESET_PERIOD)
	{
		// count the number of timer periods
		ws2812b.timer_period_counter++;
	}
	else
	{
		ws2812b.timer_period_counter = 0;
		__HAL_TIM_DISABLE(&timer2_handle);
		TIM2->CR1 = 0; // disable timer

		// disable the TIM2 Update
		__HAL_TIM_DISABLE_IT(&timer2_handle, TIM_IT_UPDATE);
		// set transfer_complete flag
		ws2812b.transfer_complete = 1;
	}

#if defined(LED5_PORT)
		LED5_PORT->BRR = LED5_PIN;
	#endif

}



static void ws2812b_set_pixel(uint8_t row, uint16_t column, uint8_t red, uint8_t green, uint8_t blue
#ifdef WS2812B_HAS_WHITE
		, uint8_t white
#endif
		)
{
#ifdef WS2812B_USE_GAMMA_CORRECTION
	// Apply gamma
	red = gammaTable[red];
	green = gammaTable[green];
	blue = gammaTable[blue];
#endif

	uint32_t calculated_column = (column *
#ifdef WS2812B_HAS_WHITE
			32
#else
			24
#endif
			);

#if defined(SETPIX_1)

	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		// write new data for pixel
		dma_bit_buffer[(calculated_column+i)] = (((((green)<<i) & 0x80)>>7)<<row) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+8+i)] = (((((red)<<i) & 0x80)>>7)<<row) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+i)] = (((((blue)<<i) & 0x80)>>7)<<row) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+i)] = (((((white)<<i) & 0x80)>>7)<<row) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif
	}

#elif defined(SETPIX_2)

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+0)] = (((green)<<0) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+0)] = (((red)<<0) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+0)] = (((blue)<<0) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+0)] = (((white)<<0) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+1)] = (((green)<<1) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+1)] = (((red)<<1) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+1)] = (((blue)<<1) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+1)] = (((white)<<1) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+2)] = (((green)<<2) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+2)] = (((red)<<2) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+2)] = (((blue)<<2) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+2)] = (((white)<<2) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+3)] = (((green)<<3) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+3)] = (((red)<<3) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+3)] = (((blue)<<3) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+3)] = (((white)<<3) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+4)] = (((green)<<4) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+4)] = (((red)<<4) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+4)] = (((blue)<<4) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+4)] = (((white)<<4) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+5)] = (((green)<<5) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+5)] = (((red)<<5) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+5)] = (((blue)<<5) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+5)] = (((white)<<5) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+6)] = (((green)<<6) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+6)] = (((red)<<6) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+6)] = (((blue)<<6) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+6)] = (((white)<<6) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

		// write new data for pixel
		dma_bit_buffer[(calculated_column+ 0+7)] = (((green)<<7) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+ 8+7)] = (((red)<<7) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		dma_bit_buffer[(calculated_column+16+7)] = (((blue)<<7) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#ifdef WS2812B_HAS_WHITE
		dma_bit_buffer[(calculated_column+24+7)] = (((white)<<7) & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

#elif defined(SETPIX_3)

		uint8_t *bit_buffer_offset = &dma_bit_buffer[calculated_column];

		// write new data for pixel
		*bit_buffer_offset++ = (green & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x40) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x20) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x10) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x08) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x04) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x02) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (green & 0x01) ? compare_pulse_logic_1 : compare_pulse_logic_0;

		*bit_buffer_offset++ = (red & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x40) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x20) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x10) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x08) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x04) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x02) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (red & 0x01) ? compare_pulse_logic_1 : compare_pulse_logic_0;

		*bit_buffer_offset++ = (blue & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x40) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x20) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x10) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x08) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x04) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x02) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (blue & 0x01) ? compare_pulse_logic_1 : compare_pulse_logic_0;

#ifdef WS2812B_HAS_WHITE
		*bit_buffer_offset++ = (white & 0x80) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x40) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x20) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x10) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x08) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x04) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x02) ? compare_pulse_logic_1 : compare_pulse_logic_0;
		*bit_buffer_offset++ = (white & 0x01) ? compare_pulse_logic_1 : compare_pulse_logic_0;
#endif

#endif
}


void ws2812b_init()
{
	ws2812b_gpio_init();
	dma_init();
	tim2_init();

	// Need to start the first transfer
	ws2812b.transfer_complete = 1;
}


void ws2812b_handle()
{
	if(ws2812b.start_transfer) {
		ws2812b.start_transfer = 0;
		ws2812b_send();
	}
}
