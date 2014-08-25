#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#define LED3_PIN                         GPIO_Pin_13
#define LED3_GPIO_PORT                   GPIOG
#define LED3_GPIO_CLK                    RCC_AHB1Periph_GPIOG  

#define LED4_PIN                         GPIO_Pin_14
#define LED4_GPIO_PORT                   GPIOG
#define LED4_GPIO_CLK                    RCC_AHB1Periph_GPIOG  

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);

#endif
