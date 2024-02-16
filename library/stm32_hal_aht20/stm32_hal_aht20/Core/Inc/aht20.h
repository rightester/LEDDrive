/*
 * aht20.h
 *
 *  Created on: Jan 31, 2024
 *      Author: 叫我最右君
 */

#ifndef INC_AHT20_H_
#define INC_AHT20_H_


#include "i2c.h"
#include <stdint.h>

#define AHT20_ADDRESS (0x38 << 1)


typedef enum {
	AHT20_UNINITIALIZED = 0u,
	AHT20_READY,
	AHT20_MEASURING,
	AHT20_MEASURED,
	AHT20_RECEIVING,
	AHT20_RECEIVED,
	AHT20_CONVERTING,
	AHT20_CONVERTED,
	AHT20_FINISHED,
} AHT20_State;


typedef struct {
	float temp;
	float humi;
	AHT20_State state;
	uint8_t addr;
	I2C_HandleTypeDef *hi2c_p;
} AHT20_HandleTypeDef;


extern AHT20_HandleTypeDef haht20;


void AHT20_Init(AHT20_HandleTypeDef *haht20_p);
void AHT20_Measure(AHT20_HandleTypeDef *haht20_p);
void AHT20_Receive(AHT20_HandleTypeDef *haht20_p);
void AHT20_ConvertValue(AHT20_HandleTypeDef *haht20_p);

void AHT20_Process(AHT20_HandleTypeDef *haht20_p);

void AHT20_MeasuredCallback(AHT20_HandleTypeDef *haht20_p);
void AHT20_ReceivedCallback(AHT20_HandleTypeDef *haht20_p);
void ATH20_ConvertedCallback(AHT20_HandleTypeDef *haht20_p);



#endif /* INC_AHT20_H_ */
