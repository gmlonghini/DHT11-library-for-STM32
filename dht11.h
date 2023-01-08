#ifndef SRC_DHT11_H_
#define SRC_DHT11_H_

#include "stm32f1xx_hal.h"

typedef struct
{
	GPIO_TypeDef* port;
	uint32_t pin;
	TIM_HandleTypeDef *htim;
	float temperature;
	float humidity;
}DHT11_t;

void DHT11_Begin(DHT11_t* dht11, GPIO_TypeDef* port, uint32_t pin, TIM_HandleTypeDef* htim);
uint8_t DHT11_Read(DHT11_t* dht11);
static void DHT11_ConfigPin(DHT11_t* dht11, uint8_t pinMode);
static void DHT11_Delay_Us(DHT11_t* dht11, uint32_t delay);
static uint8_t DHT11_Parser(DHT11_t* dht11, uint8_t data[5]);
static uint8_t DHT11_Expect_Pulse (DHT11_t* dht11, uint8_t level);

#endif /* SRC_DHT11_H_ */
