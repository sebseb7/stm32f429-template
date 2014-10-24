#include "main.h"

#include <stdlib.h>

/* manual: http://www.st.com/web/en/resource/technical/document/reference_manual/DM00031020.pdf */



#include "stm32f429i_discovery_sdram.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ioe.h"

#include "libs/mcugui/circle.h"
#include "libs/mcugui/text.h"
#include "libs/armmath.h"

static __IO uint32_t TimingDelay;
static __IO uint32_t tick;

void Delay(__IO uint32_t nTime)
{ 
	TimingDelay = nTime;

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
	y=239-y;

	*(__IO uint16_t*)(current_layer + (2*(240*x+y))) = (( (r   >> 3) & 0x001f ) << 11 | ( (g >> 2) & 0x003f ) << 5 | ((b  >> 3) & 0x001f));
}

void getLedXY(uint16_t x, uint16_t y, uint8_t* red,uint8_t* green, uint8_t* blue) {
	//	if (x >= LED_WIDTH) return;
	//	if (y >= LED_HEIGHT) return;
	y=239-y;

	uint16_t rgb565 = *(__IO uint16_t*)(current_layer + (2*(240*x+y)));

	*red = (uint8_t) (rgb565 >> 11)<<3;
	*green = (uint8_t) (rgb565 >> 5) << 2;
	*blue = (uint8_t) rgb565 << 3;
}


void invLedXY(uint16_t x, uint16_t y) {
	y=239-y;
	//	if (x >= LED_WIDTH) return;
	//	if (y >= LED_HEIGHT) return;
	uint16_t rgb565 = *(__IO uint16_t*)(current_layer + (2*(240*x+y)));

	uint8_t r = 255 - ((uint8_t) (rgb565 >> 11)<<3);
	uint8_t g = 255 - ((uint8_t) (rgb565 >> 5) << 2);
	uint8_t b = 255 - ((uint8_t) rgb565 << 3);
	*(__IO uint16_t*)(current_layer + (2*(240*x+y))) = (( (r   >> 3) & 0x001f ) << 11 | ( (g >> 2) & 0x003f ) << 5 | ((b  >> 3) & 0x001f));
}



void switch_layer(void)
{

	if(current_layer == 0xD0000000)
	{
		LTDC_Layer1->CFBAR=((uint32_t)0xD0000000);
		LTDC_ReloadConfig(LTDC_VBReload);
		while(LTDC_Layer1->CFBAR != ((uint32_t)0xD0000000));
		current_layer=0xD0000000+0x50000;
	}
	else
	{
		LTDC_Layer1->CFBAR=((uint32_t)0xD0000000+0x50000);
		LTDC_ReloadConfig(LTDC_VBReload);
		while(LTDC_Layer1->CFBAR != ((uint32_t)0xD0000000+0x50000));
		current_layer=0xD0000000;
	}

	



	//takes 1.2ms
		GPIO_SetBits(LED3_GPIO_PORT, LED3_PIN);
	for (uint32_t index = current_layer; index < current_layer+240*320*2; index+=4)
	{
		*(__IO uint32_t*)(index) = 0;
	} 
		GPIO_ResetBits(LED3_GPIO_PORT, LED3_PIN);
	
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
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static uint8_t *bzr_a, *bzr_b, *bzr_c;
static uint8_t *t_bzr_a, *t_bzr_b, *t_bzr_c;


static void init(void) {
	bzr_a = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*bzr_a));
	bzr_b = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*bzr_b));
	bzr_c = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*bzr_c));
	t_bzr_a = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*t_bzr_a));
	t_bzr_b = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*t_bzr_b));
	t_bzr_c = malloc(LED_WIDTH * LED_HEIGHT * sizeof(*t_bzr_c));

	for(int y = 0, p = 0; y < LED_HEIGHT; y++) {
		for (int x = 0; x < LED_WIDTH; x++, p++) {
			bzr_a[p] = rand() & 0xFF;
			bzr_b[p] = rand() & 0xFF;
			bzr_c[p] = rand() & 0xFF;
		}
	}
}



static void frame(void) {
	for(int y = 0, p = 0; y < LED_HEIGHT; y++) {
		for (int x = 0; x < LED_WIDTH; x++, p++) {
			/* Compute neighbor averages, with wrap-around. */
			int16_t sa = 0, sb = 0, sc = 0;
			for(int j = y -1 ; j < y + 2; j++) {
				for(int i = x - 1; i < x + 2; i++) {
					int q =
						(j < 0 ? j + LED_HEIGHT : j >= LED_HEIGHT ? j - LED_HEIGHT : j) * LED_WIDTH +
						(i < 0 ? i + LED_WIDTH : i >= LED_WIDTH ? i - LED_WIDTH : i);
					sa += bzr_a[q];
					sb += bzr_b[q];
					sc += bzr_c[q];
				}
			}
			/* This should be 9 but then it dies... */
			sa /= 9;
			sb /= 9;
			sc /= 9;

			int16_t ta = (sa * (259 + sb - sc)) >> 8;
			int16_t tb = (sb * (259 + sc - sa)) >> 8;
			int16_t tc = (sc * (259 + sa - sb)) >> 8;
			t_bzr_a[p] = MIN(255, ta);
			t_bzr_b[p] = MIN(255, tb);
			t_bzr_c[p] = MIN(255, tc);

			for(uint8_t a = 0;a<8;a++)
			{
				for(uint8_t b = 0;b<8;b++)
				{
					setLedXY(x*8+a, y*8+b, t_bzr_a[p], t_bzr_b[p], t_bzr_c[p]);
				}
			}
		}
	}    

	for(int y = 0, p = 0; y < LED_HEIGHT; y++) {
		for (int x = 0; x < LED_WIDTH; x++, p++) {
			bzr_a[p] = t_bzr_a[p];
			bzr_b[p] = t_bzr_b[p];
			bzr_c[p] = t_bzr_c[p];
		}
	}
}

int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	RCC_GetClocksFreq(&RCC_Clocks);
	/* SysTick end of count event each 1ms */
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

  static TP_STATE* TP_State; 

	LTDC_Layer1->CFBAR=((uint32_t)0xD0000000);
	LTDC_ReloadConfig(LTDC_VBReload);
	while(LTDC_Layer1->CFBAR != ((uint32_t)0xD0000000));

	uint16_t j  = 0;

	init();
	uint32_t ctick = tick;

	uint16_t touch_x = 100;
	uint16_t touch_y = 100;
	while (1)
	{
		uint32_t diff = tick-ctick;
		ctick = tick;

		j+=2;


		if(j >= 256)
		{
			j=0;
		}
		TP_State = IOE_TP_GetState();

		if(TP_State->TouchDetected)
		{
			touch_y = 239-TP_State->X;
			touch_x = TP_State->Y;
		}

		frame();
		/*draw_filledCircle(20+((sini(j*128))/234),20+((sini(j*64))/327),15.0f,0,0,0);
		draw_filledCircle(20+((sini((j+100)*128))/234),20+((sini((j+150)*64))/327),15.0f,0,255,0);
		draw_filledCircle(20+((sini((j+150)*128))/234),20+((sini((j+150)*128))/327),15.0f,0,0,255);
		draw_filledCircle(20+((sini((j+100)*128))/234),20+((sini((j+350)*64))/327),15.0f,255,0,255);
		draw_filledCircle(20+((sini((j+130)*128))/234),20+((sini((j+150)*64))/327),15.0f,0,255,255);

		draw_filledCircle(20+((sini(j*64))/234),20+((sini(j*64))/327),15.0f,255,0,0);
		draw_filledCircle(20+((sini((j+100)*64))/234),20+((sini((j+150)*64))/327),15.0f,0,255,0);
		draw_filledCircle(20+((sini((j+150)*64))/234),20+((sini((j+150)*128))/327),15.0f,0,0,255);
		draw_filledCircle(20+((sini((j+100)*64))/234),20+((sini((j+350)*128))/327),15.0f,255,0,255);
		draw_filledCircle(20+((sini((j+130)*128))/234),20+((sini((j+150)*128))/327),15.0f,0,255,255);
		*/
		draw_filledCircle(touch_x,touch_y,15.0f,0,255,255);
		draw_number_inv_8x6(100,100,10000/diff,4,'0');
		draw_number_inv_8x6(100,120,touch_x,4,'0');
		draw_number_inv_8x6(100,130,touch_y,4,'0');
		draw_number_inv_8x6(100,150,(RCC_Clocks.HCLK_Frequency / 1000000),8,'0');
		switch_layer();
	}
}

