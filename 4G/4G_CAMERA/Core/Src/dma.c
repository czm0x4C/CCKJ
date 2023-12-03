/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "dma.h"

/* USER CODE BEGIN 0 */
#include "dcmi.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/* USER CODE BEGIN 2 */
void DCMI_DMA_Init(unsigned int memaddr,unsigned short memsize,unsigned int memblen,unsigned int meminc)
{ 
    __HAL_RCC_DMA2_CLK_ENABLE();                                    //使能DMA2时钟
    __HAL_LINKDMA(&hdcmi,DMA_Handle,hdma_dcmi);        //将DMA与DCMI联系起来
	
    hdma_dcmi.Instance=DMA2_Stream1;                          //DMA2数据流1                     
    hdma_dcmi.Init.Channel=DMA_CHANNEL_1;                     //通道1
    hdma_dcmi.Init.Direction=DMA_PERIPH_TO_MEMORY;            //外设到存储器
    hdma_dcmi.Init.PeriphInc=DMA_PINC_DISABLE;                //外设非增量模式
    hdma_dcmi.Init.MemInc=meminc;                             //存储器增量模式
    hdma_dcmi.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   //外设数据长度:32位
    hdma_dcmi.Init.MemDataAlignment=memblen;                  //存储器数据长度:8/16/32位
    hdma_dcmi.Init.Mode=DMA_CIRCULAR;                         //使用循环模式 
    hdma_dcmi.Init.Priority=DMA_PRIORITY_HIGH;                //高优先级
    hdma_dcmi.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              //使能FIFO
    hdma_dcmi.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL; //使用1/2的FIFO 
    hdma_dcmi.Init.MemBurst=DMA_MBURST_SINGLE;                //存储器突发传输
    hdma_dcmi.Init.PeriphBurst=DMA_PBURST_SINGLE;             //外设突发单次传输 
    HAL_DMA_DeInit(&hdma_dcmi);                               //先清除以前的设置
    HAL_DMA_Init(&hdma_dcmi);	                                //初始化DMA
    
    //在开启DMA之前先使用__HAL_UNLOCK()解锁一次DMA,因为HAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //这两个函数一开始要先使用__HAL_LOCK()锁定DMA,而函数__HAL_LOCK()会判断当前的DMA状态是否为锁定状态，如果是
    //锁定状态的话就直接返回HAL_BUSY，这样会导致函数HAL_DMA_Statrt()和HAL_DMAEx_MultiBufferStart()后续的DMA配置
    //程序直接被跳过！DMA也就不能正常工作，为了避免这种现象，所以在启动DMA之前先调用__HAL_UNLOC()先解锁一次DMA。
    __HAL_UNLOCK(&hdma_dcmi);

	HAL_DMA_Start(&hdma_dcmi,(unsigned int)&DCMI->DR,memaddr,memsize);
}
/* USER CODE END 2 */

