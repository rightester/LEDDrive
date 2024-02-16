/*
 * aht20.c
 *
 *  Created on: Jan 31, 2024
 *      Author: 叫我最右君
 */


#include "aht20.h"
#include "usart.h"
// #include "tim.h"
#include <stdio.h>
#include <string.h>


uint8_t readBuffer[8] = {0};
AHT20_HandleTypeDef haht20 = {
	.temp = 0.0,
	.humi = 0.0,
	.state = AHT20_UNINITIALIZED,
	.addr = AHT20_ADDRESS,
	.hi2c_p = &hi2c1,
};


void AHT20_Init(AHT20_HandleTypeDef *haht20_p) {
	uint8_t readBuffer[1];
	HAL_Delay(40);
	HAL_I2C_Master_Receive(haht20_p->hi2c_p, AHT20_ADDRESS, readBuffer, 1, HAL_MAX_DELAY);
	if (!(readBuffer[0] & 1<<3)) {
		uint8_t sendBuffer[3] = {0xBE, 0x08, 0x00};
		HAL_I2C_Master_Transmit(haht20_p->hi2c_p, AHT20_ADDRESS, sendBuffer, 3, HAL_MAX_DELAY);
	}
	haht20_p->state = AHT20_READY;
}


void AHT20_Measure(AHT20_HandleTypeDef *haht20_p) {
	static uint8_t sendBuffer[3] = {0xAC, 0x33, 0x00};
	HAL_I2C_Master_Transmit_DMA(haht20_p->hi2c_p, AHT20_ADDRESS, sendBuffer, 3);
}


void AHT20_Receive(AHT20_HandleTypeDef *haht20_p) {
	for (int i=0; ; ) {
		HAL_I2C_Master_Receive_DMA(haht20_p->hi2c_p, AHT20_ADDRESS, readBuffer, 6);
		if ( (readBuffer[0] & 1<<7) == 0x00 ) {
			return;
		}
		i++;
		if (i>3) {
			break;
		}
		HAL_Delay(75);
	}
}


// uint32_t __REV(uint32_t) 可以硬件实现大小端序转换，以便进行数值运算，arm架构是小端序的，aht20传递过来的数值是大端序的
// 20位的数据大于16位，只好用更大点的32位的类型uint32_t
// 温度 取了readBuffer下标0至4共32位数据作为u32解引用(其中真正需要的*数据位*是[8..28]共20位数据)，__REV进行大小端序转换，
//     这样得到的结果就可以进行期望的数学运算和位运算；
//     然后位运算保留其中的[8..28]数据位，清0其他位，再整体右移4位，获得真正的期望的小端有效数值
// 湿度 类同温度，需取得的数据位为readBuffer的下标2至6的字节共32位数其中的[12..32]共20位数据，运算后不需要移位
// 得到转换后的数据就可以用之于后面的数学运算了
// 如果没有 #include <cmsis_gcc.h> 那就按照#else中的数学运算的方式来算
void AHT20_ConvertValue(AHT20_HandleTypeDef *haht20_p) {
	float humi, temp = 0;
	#ifdef __CMSIS_GCC_H
	humi = (__REV(*(uint32_t*)readBuffer) & 0x00fffff0) >> 4;
	temp = __REV(*(uint32_t*)(readBuffer+2)) & 0x000fffff;
	#else
	humi = ((uint32_t)readBuffer[3] >> 4) + ((uint32_t)readBuffer[2] << 4) + ((uint32_t)readBuffer[1] << 12);
	temp = (((uint32_t)readBuffer[3] & 0x0F) << 16) + (((uint32_t)readBuffer[4]) << 8) + (uint32_t)readBuffer[5];
	#endif
	haht20_p->humi = humi / (1<<20) * 100;
	haht20_p->temp = temp / (1<<20) * 200 - 50;
	haht20_p->state = AHT20_CONVERTED;
}


__weak void AHT20_Process(AHT20_HandleTypeDef *haht20_p) {
	switch(haht20_p->state) {
	case AHT20_READY:
		haht20_p->state = AHT20_MEASURING;
		AHT20_Measure(haht20_p);
		HAL_Delay(75);
		break;
	case AHT20_MEASURED:
		haht20_p->state = AHT20_RECEIVING;
		AHT20_Receive(haht20_p);
		break;
	case AHT20_RECEIVED:
		haht20_p->state = AHT20_CONVERTING;
		// HAL_TIM_Base_Init(&htim3);
		// HAL_TIM_Base_Start(&htim3);
		AHT20_ConvertValue(haht20_p);
		// HAL_TIM_Base_Stop(&htim3);
		// static char str[50];
		// int counter = __HAL_TIM_GET_COUNTER(&htim3);
		// sprintf(str, "Counter: %d\n", counter);
		// HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
		break;
	case AHT20_CONVERTED:
		ATH20_ConvertedCallback(haht20_p);
		break;
	case AHT20_FINISHED:
		haht20_p->state = AHT20_READY;
		break;
	default:
		break;
	}
}


void AHT20_MeasuredCallback(AHT20_HandleTypeDef *haht20_p) {
	switch(haht20_p->state) {
	case AHT20_MEASURING:
		haht20_p->state = AHT20_MEASURED;
	default:
		return;
	}
}/*
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == haht20.hi2c_p) {
		AHT20_MeasuredCallback(&haht20);
	}
}
*/


void AHT20_ReceivedCallback(AHT20_HandleTypeDef *haht20_p) {
	switch (haht20_p->state) {
	case AHT20_RECEIVING:
		haht20_p->state = AHT20_RECEIVED;
	default:
		return;
	}
}/*
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == haht20.hi2c_p) {
		AHT20_ReceivedCallback(&haht20);
	}
}
*/


__weak void ATH20_ConvertedCallback(AHT20_HandleTypeDef *haht20_p) {
	  static char str[50];
	  sprintf(str, "温度：%.1f℃，湿度：%.0f%%\n", haht20_p->temp, haht20_p->humi);
	  HAL_UART_Transmit(&huart3, (uint8_t*)str, strlen(str), HAL_MAX_DELAY);
	  haht20_p->state = AHT20_FINISHED;
}

