#include "main.h"

#include <stdlib.h>

/* manual: http://www.st.com/web/en/resource/technical/document/reference_manual/DM00031020.pdf */

#include "stm32f429i_discovery_sdram.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"

#include "libs/mcugui/button.h"
#include "libs/mcugui/circle.h"
#include "libs/mcugui/text.h"
#include "libs/armmath.h"

static __IO uint32_t TimingDelay;
static __IO uint32_t tick;

void Delay(__IO uint32_t nTime)
{ 
	TimingDelay = nTime*10;

	while(TimingDelay != 0);
}

void TimingDelay_Decrement(void)
{
	if (TimingDelay != 0)
	{ 
		TimingDelay--;
	}
	tick++;
}

static uint32_t current_layer=0xD0000000+0x50000;

void setLedXY(uint16_t x,uint16_t y, uint8_t r,uint8_t g,uint8_t b)
{
	if (x >= LED_WIDTH) return;
	if (y >= LED_HEIGHT) return;
	y=239-y;

	*(__IO uint16_t*)(current_layer + (2*(240*x+y))) = (( (r   >> 3) & 0x001f ) << 11 | ( (g >> 2) & 0x003f ) << 5 | ((b  >> 3) & 0x001f));
}

void getLedXY(uint16_t x, uint16_t y, uint8_t* red,uint8_t* green, uint8_t* blue) {
	if (x >= LED_WIDTH) return;
	if (y >= LED_HEIGHT) return;
	y=239-y;

	uint16_t rgb565 = *(__IO uint16_t*)(current_layer + (2*(240*x+y)));

	*red = (uint8_t) (rgb565 >> 11)<<3;
	*green = (uint8_t) (rgb565 >> 5) << 2;
	*blue = (uint8_t) rgb565 << 3;
}


void invLedXY(uint16_t x, uint16_t y) {
	if (x >= LED_WIDTH) return;
	if (y >= LED_HEIGHT) return;
	y=239-y;
	uint16_t rgb565 = *(__IO uint16_t*)(current_layer + (2*(240*x+y)));

	uint8_t r = 255 - ((uint8_t) (rgb565 >> 11)<<3);
	uint8_t g = 255 - ((uint8_t) (rgb565 >> 5) << 2);
	uint8_t b = 255 - ((uint8_t) rgb565 << 3);
	*(__IO uint16_t*)(current_layer + (2*(240*x+y))) = (( (r   >> 3) & 0x001f ) << 11 | ( (g >> 2) & 0x003f ) << 5 | ((b  >> 3) & 0x001f));
}

/*void dma_mem_clear(uint32_t base)
{
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 , ENABLE);
		volatile unsigned long ulToCopy = 0;
		DMA2_Stream0->NDTR = 64;                                    // the number of long word transfers to be made (max. 0xffff)
		DMA2_Stream0->PAR = (unsigned long)&ulToCopy;                           // address of long word to be transfered
		DMA2_Stream0->M0AR = base;                                // address of first destination long word
		DMA2_Stream0->CR = (DMA_MemoryInc_Enable | DMA_PeripheralDataSize_Word | DMA_MemoryDataSize_Word | DMA_Priority_Medium | DMA_DIR_MemoryToMemory | DMA_SxCR_TCIE); // set up DMA operation
		DMA2_Stream0->CR |= DMA_SxCR_EN;                                        // start operation
		while ((DMA2->LISR & DMA_IT_TCIF0) == 0) { };        // wait until the DMA transfer has terminated
		DMA2->LIFCR = (DMA_IT_TCIF0 | DMA_IT_HTIF0 | DMA_IT_DMEIF0 | DMA_IT_FEIF0 | DMA_IT_DMEIF0); // clear flags
		DMA2->LISR = 0;
		DMA2_Stream0->CR = 0;                                                   // mark that the DMA stream is free for use again
}
*/


#define LAYER_A  0xD0000000
#define LAYER_B (0xD0000000+0x50000)
	static TP_STATE* TP_State; 
	
	uint16_t touch_x = 100;
	uint16_t touch_y = 100;

void switch_layer(void)
{
	static uint32_t ij = 0;

	ij++;


	if(current_layer == LAYER_A)
	{
		LTDC_Layer1->CFBAR=((uint32_t)LAYER_A);
		LTDC_ReloadConfig(LTDC_VBReload);
		if((ij % 8) == 0)
		{
			TP_State = IOE_TP_GetState();

			if(TP_State->TouchDetected)
			{
				touch_y = 239-TP_State->X;
				touch_x = TP_State->Y;
			}
		}
		while(LTDC_Layer1->CFBAR != ((uint32_t)LAYER_A));
		current_layer=LAYER_B;
	}
	else
	{
		LTDC_Layer1->CFBAR=((uint32_t)LAYER_B);
		LTDC_ReloadConfig(LTDC_VBReload);
		if((ij % 8) == 0)
		{
			TP_State = IOE_TP_GetState();

			if(TP_State->TouchDetected)
			{
				touch_y = 239-TP_State->X;
				touch_x = TP_State->Y;
			}
		}
		while(LTDC_Layer1->CFBAR != ((uint32_t)LAYER_B));
		current_layer=LAYER_A;
	}





	//takes 1.2ms
//	dma_mem_clear(current_layer);
	
	for (uint32_t index = current_layer; index < current_layer+240*320*2; index+=4)
	{
		*(__IO uint32_t*)(index) = 0;
	} 

	// takes 6ms
	//memset(current_layer, 0, 240*320*2);

	// takes 1.2ms to clear the framebuffer == 220k cylces or 3cycles per pixel
	/*	DMA2D_InitTypeDef      DMA2D_InitStruct;
		DMA2D_DeInit();
		DMA2D_InitStruct.DMA2D_Mode = DMA2D_R2M;       
		DMA2D_InitStruct.DMA2D_CMode = DMA2D_RGB565;      
		DMA2D_InitStruct.DMA2D_OutputGreen = 0;      
		DMA2D_InitStruct.DMA2D_OutputBlue = 0;     
		DMA2D_InitStruct.DMA2D_OutputRed = 0;                
		DMA2D_InitStruct.DMA2D_OutputAlpha = 0x0F;                  
		DMA2D_InitStruct.DMA2D_OutputMemoryAdd = current_layer;                
		DMA2D_InitStruct.DMA2D_OutputOffset = 0;                
		DMA2D_InitStruct.DMA2D_NumberOfLine = 240;            
		DMA2D_InitStruct.DMA2D_PixelPerLine = 320;
		DMA2D_Init(&DMA2D_InitStruct); 
		DMA2D_StartTransfer();

		while(DMA2D_GetFlagStatus(DMA2D_FLAG_TC) == RESET)
		{
		}*/
	// __WFE()

}

int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	RCC_GetClocksFreq(&RCC_Clocks);
	/* SysTick end of count event each 0.1ms */
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 10000);

	RCC_AHB1PeriphClockCmd(LED3_GPIO_CLK, ENABLE);
	RCC_AHB1PeriphClockCmd(LED3_GPIO_CLK, ENABLE);

	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = LED3_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LED4_PIN;
	GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);

	LCD_Init();
	LCD_LayerInit();
	LTDC_Cmd(ENABLE);
	LCD_SetLayer(LCD_FOREGROUND_LAYER);
	LCD_Clear(LCD_COLOR_WHITE);
	LCD_SetTransparency(0);
	LTDC_ReloadConfig(LTDC_IMReload);

	SDRAM_Init();
	SDRAM_GPIOConfig();
	FMC_SDRAMWriteProtectionConfig(FMC_Bank2_SDRAM,DISABLE); 

	if (IOE_Config() == IOE_OK)
	{
	}


	LTDC_Layer1->CFBAR=((uint32_t)0xD0000000);
	LTDC_ReloadConfig(LTDC_VBReload);
	while(LTDC_Layer1->CFBAR != ((uint32_t)0xD0000000));

	uint16_t j  = 0;

	uint32_t ctick = tick;
	uint32_t diff_buf=0;
	uint32_t diff=0;

	while (1)
	{
//		GPIO_SetBits(LED3_GPIO_PORT, LED3_PIN);
//		GPIO_ResetBits(LED3_GPIO_PORT, LED3_PIN);
		diff += tick-ctick;
		ctick = tick;

		j+=1;


		if(j >= 256)
		{
			j=0;
		}
		if((j % 32) == 0)
		{
			diff_buf=diff/32;
			diff=0;
		}

		draw_filledCircle(20+((sini(j*128))/234),20+((sini(j*64))/327),15.0f,0,0,0);
		draw_filledCircle(20+((sini((j+100)*128))/234),20+((sini((j+150)*64))/327),15.0f,0,255,0);
		draw_filledCircle(20+((sini((j+150)*128))/234),20+((sini((j+150)*128))/327),15.0f,0,0,255);
		draw_filledCircle(20+((sini((j+100)*128))/234),20+((sini((j+350)*64))/327),15.0f,255,0,255);
		draw_filledCircle(20+((sini((j+130)*128))/234),20+((sini((j+150)*64))/327),15.0f,0,255,255);

		draw_filledCircle(20+((sini(j*64))/234),20+((sini(j*64))/327),15.0f,255,0,0);
		draw_filledCircle(20+((sini((j+100)*64))/234),20+((sini((j+150)*64))/327),15.0f,0,255,0);
		draw_filledCircle(20+((sini((j+150)*64))/234),20+((sini((j+150)*128))/327),15.0f,0,0,255);
		draw_filledCircle(20+((sini((j+100)*64))/234),20+((sini((j+350)*128))/327),15.0f,255,0,255);
		draw_filledCircle(20+((sini((j+130)*128))/234),20+((sini((j+150)*128))/327),15.0f,0,255,255);

		draw_filledCircle(touch_x,touch_y,15.0f,0,255,255);
		draw_number_inv_8x6(100,100,10000/diff_buf,4,'0');
		draw_number_inv_8x6(100,120,touch_x,4,'0');
		draw_number_inv_8x6(100,130,touch_y,4,'0');

		draw_button(30,30,200,1,"Textbutton",255,0,0,0,255,0);
		draw_button(30,70,200,1,"Textbutton",255,0,0,0,255,0);
		draw_button(30,110,200,1,"Textbutton",255,0,0,0,255,0);
		draw_button(30,150,200,1,"Textbutton",255,0,0,0,255,0);
		switch_layer();
	}
}

