/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "string.h"

#define TBUF_SIZE 16
#define RBUF_SIZE 16

static  uint8_t TxBuff [TBUF_SIZE];
static  uint8_t RxBuff [RBUF_SIZE];

static volatile uint8_t TxWriteIndex = 0 ;
static volatile uint8_t TxReadIndex  = 0 ;
static volatile uint8_t RxWriteIndex = 0 ;
static volatile uint8_t RxReadIndex  = 0 ;

static char ti_restart = 1 ;

unsigned char *aux=RxBuff;

uint8_t UART3Rx_Buffer[128];

uint8_t Rx_Buffer[128];
int received = 0;

volatile uint8_t UART3Rx_index = 0;

/* USER CODE END 0 */

UART_HandleTypeDef huart3;

/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration    
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */
int put_char(unsigned char c){
	if(get_Txbuffsize() >= TBUF_SIZE){
		return(-ENOBUFS);
	}
	TxBuff [TxWriteIndex & (TBUF_SIZE - 1)] = c ;
	TxWriteIndex++;
	if (ti_restart){ 
		ti_restart = 0;
	  HAL_UART_Transmit_IT(&huart3, &TxBuff[TxReadIndex & (TBUF_SIZE - 1)], 1);
		TxReadIndex++;
	}
	return 0;
}

int get_char(void){
	if(get_Rxbuffsize() == 0){
		return(-ENODATA);
	}
	else{
		return(RxBuff[(RxReadIndex++) & (RBUF_SIZE - 1)]);
	}
}

uint8_t get_Rxbuffsize(void){
	return(RxWriteIndex - RxReadIndex);
}

uint8_t get_Txbuffsize(void){
	return(TxWriteIndex - TxReadIndex);
}

void init_UART3(){
	HAL_UART_Receive_IT(&huart3, &RxBuff[RxWriteIndex], 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
	if (huart->Instance == USART3){
		RxWriteIndex++;
		HAL_UART_Receive_IT(&huart3, &RxBuff[RxWriteIndex  & (RBUF_SIZE-1)], 1);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(get_Txbuffsize() != 0){
		ti_restart = 0;
		HAL_UART_Transmit_IT(&huart3, &TxBuff[TxReadIndex & (TBUF_SIZE - 1)], 1);
		TxReadIndex++;
	}
	else{
		ti_restart = 1;
	}
}

void newMessage(){
	static int local_index = 0;
	int out_index = 0;
	while(local_index != RxWriteIndex){
		Rx_Buffer[out_index] = RxBuff[local_index];
		out_index++;
		local_index++;
		local_index &= ~(1<<7);
		received = 1;
	}
	Rx_Buffer[out_index] = '\0';
}

int fputc(int ch, FILE *f){
	HAL_UART_Transmit(&huart3, (uint8_t*)&ch, 1, 100);
	return ch;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
