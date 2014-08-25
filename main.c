#include "main.h"

static __IO uint32_t TimingDelay;

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
}


int main(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	RCC_GetClocksFreq(&RCC_Clocks);
	/* SysTick end of count event each 1ms */
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

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


	while (1)
	{
		GPIO_SetBits(LED3_GPIO_PORT, LED3_PIN);
		Delay(50);
		GPIO_SetBits(LED4_GPIO_PORT, LED4_PIN);
		Delay(50);
		GPIO_ResetBits(LED3_GPIO_PORT, LED3_PIN);
		Delay(50);
		GPIO_ResetBits(LED4_GPIO_PORT, LED4_PIN);
		Delay(50);
	}
}

