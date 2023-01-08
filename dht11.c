#include "dht11.h"

#define DHT11_ERROR 0
#define DHT11_OK 1
#define DHT11_PIN_INPUT 0
#define DHT11_PIN_OUTPUT 1
#define DHT11_TIMEOUT 100

void DHT11_Begin(DHT11_t *dht11, GPIO_TypeDef *port, uint32_t pin, TIM_HandleTypeDef *htim)
{
	dht11->port = port;
	dht11->pin = pin;
	dht11->htim = htim;
	HAL_TIM_Base_Start(dht11->htim);
}

uint8_t DHT11_Read(DHT11_t *dht11)
{
	uint8_t expectPulse;
	uint8_t data[5];
	uint8_t nData = 0;
	int dataIndex = 0;

	DHT11_ConfigPin(dht11, DHT11_PIN_OUTPUT);
	HAL_GPIO_WritePin(dht11->port, dht11->pin, 0);
	HAL_Delay(20); //According to datasheet pulls down voltage at least 18ms
	__disable_irq(); //Timing critical area
	DHT11_ConfigPin(dht11, DHT11_PIN_INPUT);
	DHT11_Delay_Us(dht11, 40);//According to datasheet wait DHT response (20-40 us)

	if (HAL_GPIO_ReadPin(dht11->port, dht11->pin) == 1) //At this point, if the line is high there's something wrong
	{
		__enable_irq();
		return DHT11_ERROR;
	}

	__HAL_TIM_SET_COUNTER(dht11->htim, 0);

	if (DHT11_Expect_Pulse(dht11, 1) == DHT11_ERROR)//According to datasheet DHT keeps the line down for 80us
	{
		__enable_irq();
		return DHT11_ERROR;
	}

	__HAL_TIM_SET_COUNTER(dht11->htim, 0);

	if (DHT11_Expect_Pulse(dht11, 0) == DHT11_ERROR) //According to datasheet DHT pulls up voltage and keeps it for 80us
	{
		__enable_irq();
		return DHT11_ERROR;
	}

	__HAL_TIM_SET_COUNTER(dht11->htim, 0);
	expectPulse = 1;
	//Data reading begin
	while (nData < 40)
	{
		if (DHT11_Expect_Pulse(dht11, expectPulse) == DHT11_ERROR)
		{
			__enable_irq();
			return DHT11_ERROR;
		}
		else
		{
			if (expectPulse == 0)
			{
				data[dataIndex] = data[dataIndex] << 1;

				//According to datasheet 26-28us means data "0" and 70us means data "1", so 50us is a good threshhold
				if (__HAL_TIM_GET_COUNTER(dht11->htim) > 50)
				{
					data[dataIndex] |= 0x01;
				}

				nData++;

				if (!(nData % 8))
				{
					dataIndex++;
				}
			}
			expectPulse ^= 1;
			__HAL_TIM_SET_COUNTER(dht11->htim, 0);
		}
	}

	__enable_irq(); //End of critical timing area

	if (DHT11_Parser(dht11, data) == DHT11_ERROR)
	{
		return DHT11_ERROR;
	}

	return DHT11_OK;
}

uint8_t DHT11_Parser(DHT11_t *dht11, uint8_t data[5])
{
	//data[4] is the checksum
	if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
	{
		return DHT11_ERROR;
	}

	dht11->temperature = 0.0;
	dht11->temperature = data[2];

	if (data[3] & 0x80)
	{
		dht11->temperature = -1 - dht11->temperature;
	}

	dht11->temperature += (data[3] & 0x0f) * 0.1;

	dht11->humidity = data[0] + data[1] * 0.1;

	return DHT11_OK;
}

void DHT11_ConfigPin(DHT11_t *dht11, uint8_t pinMode)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = dht11->pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Mode = pinMode == DHT11_PIN_OUTPUT ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;

	HAL_GPIO_Init(dht11->port, &GPIO_InitStruct);
}

void DHT11_Delay_Us(DHT11_t *dht11, uint32_t delay)
{
	__HAL_TIM_SET_COUNTER(dht11->htim, 0);
	while (__HAL_TIM_GET_COUNTER(dht11->htim) < delay);
}

uint8_t DHT11_Expect_Pulse(DHT11_t *dht11, uint8_t level)
{
	while (__HAL_TIM_GET_COUNTER(dht11->htim) < DHT11_TIMEOUT)
	{
		if (HAL_GPIO_ReadPin(dht11->port, dht11->pin) == level)
		{
			return DHT11_OK;
		}
	}
	return DHT11_ERROR;
}
