#ifndef MAIN_H_
#define MAIN_H_

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"

#define LED_WIDTH 320
#define LED_HEIGHT 240

void TimingDelay_Decrement(void);
void Delay(__IO uint32_t nTime);
void setLedXY(uint16_t x,uint16_t y, uint8_t r,uint8_t g,uint8_t b);
void invLedXY(uint16_t x, uint16_t y);
void getLedXY(uint16_t x, uint16_t y, uint8_t* r,uint8_t* g, uint8_t* b);

float pythagorasf( float side1, float side2 );

#endif
