/*
 * DHT22.c
 *
 *  Created on: Feb 20, 2020
 *      Author: mrhause, Philip Kiehnle
 *      Todo: add timeout
 */

#include "DHT22.h"
static GPIO_TypeDef* wire_Port;
static uint16_t wire_Pin;
GPIO_InitTypeDef GPIO_InitStruct;
uint8_t Hum_byte1, Hum_byte2, Temp_byte1, Temp_byte2, err;
uint16_t Sum;

static void TIM_Config(void)
{
	RCC_ClkInitTypeDef myCLKtypeDef;
	uint32_t clockSpeed;
	uint32_t flashLatencyVar;
	HAL_RCC_GetClockConfig(&myCLKtypeDef, &flashLatencyVar);
	if(myCLKtypeDef.APB1CLKDivider == RCC_HCLK_DIV1)
	{
		clockSpeed = HAL_RCC_GetPCLK1Freq();
	}
	else
	{
		clockSpeed = HAL_RCC_GetPCLK1Freq()*2;
	}
	clockSpeed *= 0.000001;
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;  // 0x1
	TIM3->CR1 &= ~(0x0010);
	TIM3->CR1 &= ~(0x0001);
	TIM3->CR1 &= ~(1UL << 2);
	TIM3->CR1 |= (1UL << 3);
	TIM3->PSC = clockSpeed-1;
	TIM3->ARR = 10-1;
	TIM3->EGR = 1;
	TIM3->SR &= ~(0x0001);
}
//microsecond delay
static void delay_my(uint32_t uSecDelay)
{
	TIM3->ARR = uSecDelay-1;
	TIM3->SR &= ~(0x0001);
	TIM3->CR1 |= 1UL;
	while((TIM3->SR&0x0001) != 1);
}

void DHT22_Init(GPIO_TypeDef* Port,uint16_t Pin){
	wire_Port = Port;
	wire_Pin = Pin;
	TIM_Config();;
}

void gpio_output(){
	GPIO_InitStruct.Pin = wire_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(wire_Port, &GPIO_InitStruct);
}

void gpio_input(){
	GPIO_InitStruct.Pin = wire_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(wire_Port, &GPIO_InitStruct);
}

void ONE_WIRE_START(){
	gpio_output();
	HAL_GPIO_WritePin(wire_Port, wire_Pin, GPIO_PIN_RESET);
	delay_my(500);
	HAL_GPIO_WritePin(wire_Port, wire_Pin, GPIO_PIN_SET);
	delay_my(30);
	gpio_input();
}
void ONE_WIRE_Responsee(){
	delay_my(40);
	if(!(HAL_GPIO_ReadPin(wire_Port, wire_Pin))) //check if it's low state
	{
		delay_my(80);
		if(HAL_GPIO_ReadPin(wire_Port, wire_Pin)){ //if after 80us is high state response is correct
			err=1;
		}
		while(HAL_GPIO_ReadPin(wire_Port, wire_Pin)); //wait until high state
	}
}
uint8_t ONE_WIRE_Read_Data(){
	uint8_t i,j;
	for(j=0;j<8;j++)
	{
		while(!(HAL_GPIO_ReadPin(wire_Port, wire_Pin))); //wait until pin is low
		delay_my(50);
		if(HAL_GPIO_ReadPin(wire_Port, wire_Pin)==0){ //if after 26-28us is still 0 means bit is 0
			i&= ~(1<<(7-j));
		}
		else{ //otherwise is 1
			i|=(1<<(7-j));
		}
		while(HAL_GPIO_ReadPin(wire_Port, wire_Pin)); //
	}
	return i;
}

uint8_t DHT22_Get_Data(uint16_t *Temp,uint16_t *Hum){
	ONE_WIRE_START();
	ONE_WIRE_Responsee();
	Hum_byte1 = ONE_WIRE_Read_Data();
	Hum_byte2 = ONE_WIRE_Read_Data();
	Temp_byte1 = ONE_WIRE_Read_Data();
	Temp_byte2 = ONE_WIRE_Read_Data();
	Sum = ONE_WIRE_Read_Data();
	*Temp = (Temp_byte1<<8)|Temp_byte2;
	*Hum = (Hum_byte1<<8)|Hum_byte2;
	return err;
}
