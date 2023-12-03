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
    __HAL_RCC_DMA2_CLK_ENABLE();                                    //ʹ��DMA2ʱ��
    __HAL_LINKDMA(&hdcmi,DMA_Handle,hdma_dcmi);        //��DMA��DCMI��ϵ����
	
    hdma_dcmi.Instance=DMA2_Stream1;                          //DMA2������1                     
    hdma_dcmi.Init.Channel=DMA_CHANNEL_1;                     //ͨ��1
    hdma_dcmi.Init.Direction=DMA_PERIPH_TO_MEMORY;            //���赽�洢��
    hdma_dcmi.Init.PeriphInc=DMA_PINC_DISABLE;                //���������ģʽ
    hdma_dcmi.Init.MemInc=meminc;                             //�洢������ģʽ
    hdma_dcmi.Init.PeriphDataAlignment=DMA_PDATAALIGN_WORD;   //�������ݳ���:32λ
    hdma_dcmi.Init.MemDataAlignment=memblen;                  //�洢�����ݳ���:8/16/32λ
    hdma_dcmi.Init.Mode=DMA_CIRCULAR;                         //ʹ��ѭ��ģʽ 
    hdma_dcmi.Init.Priority=DMA_PRIORITY_HIGH;                //�����ȼ�
    hdma_dcmi.Init.FIFOMode=DMA_FIFOMODE_ENABLE;              //ʹ��FIFO
    hdma_dcmi.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL; //ʹ��1/2��FIFO 
    hdma_dcmi.Init.MemBurst=DMA_MBURST_SINGLE;                //�洢��ͻ������
    hdma_dcmi.Init.PeriphBurst=DMA_PBURST_SINGLE;             //����ͻ�����δ��� 
    HAL_DMA_DeInit(&hdma_dcmi);                               //�������ǰ������
    HAL_DMA_Init(&hdma_dcmi);	                                //��ʼ��DMA
    
    //�ڿ���DMA֮ǰ��ʹ��__HAL_UNLOCK()����һ��DMA,��ΪHAL_DMA_Statrt()HAL_DMAEx_MultiBufferStart()
    //����������һ��ʼҪ��ʹ��__HAL_LOCK()����DMA,������__HAL_LOCK()���жϵ�ǰ��DMA״̬�Ƿ�Ϊ����״̬�������
    //����״̬�Ļ���ֱ�ӷ���HAL_BUSY�������ᵼ�º���HAL_DMA_Statrt()��HAL_DMAEx_MultiBufferStart()������DMA����
    //����ֱ�ӱ�������DMAҲ�Ͳ�������������Ϊ�˱���������������������DMA֮ǰ�ȵ���__HAL_UNLOC()�Ƚ���һ��DMA��
    __HAL_UNLOCK(&hdma_dcmi);

	HAL_DMA_Start(&hdma_dcmi,(unsigned int)&DCMI->DR,memaddr,memsize);
}
/* USER CODE END 2 */

