/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
/*
	StartCmdAnalysis	������յ�����
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "usart.h"
#include "ringBuffer.h"
#include "usbd_cdc_if.h"
#include "flash.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
const char AT_CDM_1[] = "AT+CREG?\r\n";
const char AT_CDM_2[] = "AT+CGATT?\r\n";
const char AT_CDM_3[] = "AT+CIPMUX=0\r\n";
const char AT_CDM_4[] = "AT+CIPMODE=1\r\n";
const char AT_CDM_5[] = "AT+CSTT\r\n";
const char AT_CDM_6[] = "AT+CIICR\r\n";
const char AT_CDM_7[] = "AT+CIFSR\r\n";
const char AT_CDM_8[] = "AT+CIPSTATUS\r\n";
char AT_CDM_9[50];
//const char AT_CDM_9[] = "AT+CIPSTART=\"TCP\",47.108.205.185,8000\r\n";

//const char AT_CDM_9[] = "AT+CIFSR\r\n";
//const char AT_CDM_10[] = "AT+CIFSR\r\n";
//const char AT_CDM_11[] = "AT+CIFSR\r\n";
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for cmdAnalysis */
osThreadId_t cmdAnalysisHandle;
const osThreadAttr_t cmdAnalysis_attributes = {
  .name = "cmdAnalysis",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal7,
};
/* Definitions for sendDataTask */
osThreadId_t sendDataTaskHandle;
const osThreadAttr_t sendDataTask_attributes = {
  .name = "sendDataTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal6,
};
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void vcpDataAnalysis(_RingBuffer *ringbuffer);
static void usart1DataAnalysis(_RingBuffer *ringbuffer);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartCmdAnalysis(void *argument);
void StartSendDataTask(void *argument);
void StartLedTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	taskENTER_CRITICAL();//�������ٽ���
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of cmdAnalysis */
  cmdAnalysisHandle = osThreadNew(StartCmdAnalysis, NULL, &cmdAnalysis_attributes);

  /* creation of sendDataTask */
  sendDataTaskHandle = osThreadNew(StartSendDataTask, NULL, &sendDataTask_attributes);

  /* creation of ledTask */
  ledTaskHandle = osThreadNew(StartLedTask, NULL, &ledTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
	taskEXIT_CRITICAL();//�˳��ٽ���
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
		usbOK = 1;
		
//		if(jpeg_data_ok == 1)
//		{
//			while(CDC_Transmit_HS((unsigned char *)dcmi_data_buff,jpeg_data_len * 4) != USBD_OK){};
//			jpeg_data_ok = 2;
//		}
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartCmdAnalysis */
/**
* @brief Function implementing the cmdAnalysis thread.
* @param argument: Not used
* @retval None
*/
char anlysis = 0;
/* USER CODE END Header_StartCmdAnalysis */
void StartCmdAnalysis(void *argument)
{
  /* USER CODE BEGIN StartCmdAnalysis */
	TickType_t	PreviousWakeTime = xTaskGetTickCount(); 
  /* Infinite loop */
//	WriteFlashData(ADDR_FLASH_SECTOR_7,(unsigned char *)deviceAttributeInfo.picFormat,sizeof(_deviceInfo));
	memcpy((unsigned char *)deviceAttributeInfo.picFormat,(uint32_t*)ADDR_FLASH_SECTOR_7,sizeof(_deviceInfo));
  for(;;)
  {
		vcpDataAnalysis(&VCP_RxRingBuffer);
		if(anlysis == 1)
			usart1DataAnalysis(&UART_RxRingBuffer);
    vTaskDelayUntil(&PreviousWakeTime, configTICK_RATE_HZ/100);//ָ���������ִ������Ϊ10Hz
  }
  /* USER CODE END StartCmdAnalysis */
}

/* USER CODE BEGIN Header_StartSendDataTask */
/**
* @brief Function implementing the sendDataTask thread.
* @param argument: Not used
* @retval None
*/
char takePictureFlag = 0;
char openMotoFlag = 0;
/* USER CODE END Header_StartSendDataTask */
void StartSendDataTask(void *argument)
{
  /* USER CODE BEGIN StartSendDataTask */
	char frameBuffer[100];
	unsigned int sendTcpDataLen = 0;
	HAL_UART_Transmit(&huart1,(const unsigned char *)"AT\r\n",4,0xFFFFFFFF);
	osDelay(2000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)"AT\r\n",4,0xFFFFFFFF);
	osDelay(2000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)"AT\r\n",4,0xFFFFFFFF);
	osDelay(2000);
	
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_1,strlen(AT_CDM_1),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_2,strlen(AT_CDM_2),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_3,strlen(AT_CDM_3),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_4,strlen(AT_CDM_4),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_5,strlen(AT_CDM_5),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_6,strlen(AT_CDM_6),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_7,strlen(AT_CDM_7),0xFFFFFFFF);
	osDelay(1000);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_8,strlen(AT_CDM_8),0xFFFFFFFF);
	osDelay(1000);
	memset(AT_CDM_9,0,sizeof(AT_CDM_9));
	sprintf(AT_CDM_9,"AT+CIPSTART=\"TCP\",%s,%d\r\n",deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort);
	HAL_UART_Transmit(&huart1,(const unsigned char *)AT_CDM_9,strlen(AT_CDM_9),0xFFFFFFFF);
	osDelay(5000);
	anlysis = 1;
	
	
	
	memset(frameBuffer,0,100);/* ������� */
	sendTcpDataLen = 1;
	frameBuffer[0] = sendTcpDataLen;
	frameBuffer[1] = sendTcpDataLen>>8;
	frameBuffer[2] = sendTcpDataLen>>16;
	frameBuffer[3] = sendTcpDataLen>>24;
	frameBuffer[4] = SET_CAMERA_DEVICE_FLAG;
	HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
		
	memset(frameBuffer,0,100);/* ������� */
	sendTcpDataLen = 1 + strlen(deviceAttributeInfo.deviceID);
	frameBuffer[0] = sendTcpDataLen;
	frameBuffer[1] = sendTcpDataLen>>8;
	frameBuffer[2] = sendTcpDataLen>>16;
	frameBuffer[3] = sendTcpDataLen>>24;

	frameBuffer[4] = SET_CAMERA_DEVICE_ID;

	memcpy(&frameBuffer[5],&deviceAttributeInfo.deviceID,strlen(deviceAttributeInfo.deviceID));
	HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5+strlen(deviceAttributeInfo.deviceID),0xFFFFFFFF);
	
  /* Infinite loop */
  for(;;)
  {
		/* ����TCP������ */
		memset(frameBuffer,0,100);/* ������� */
		sendTcpDataLen = 1;
		frameBuffer[0] = sendTcpDataLen;
		frameBuffer[1] = sendTcpDataLen>>8;
		frameBuffer[2] = sendTcpDataLen>>16;
		frameBuffer[3] = sendTcpDataLen>>24;

		frameBuffer[4] = HERAT_BEAT_PACK;
		HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
		
/*******************************************************************************����**********************************************************************************************/
		if(takePictureFlag == 1)
		{
			__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, deviceAttributeInfo.ledFlashBrightness);
			
			unsigned int tryPic = deviceAttributeInfo.takePictureDelayTime * 10;
			while(tryPic)
			{
					jpeg_data_ok = 2;
					while(jpeg_data_ok != 1);
					vTaskDelay(100 /portTICK_PERIOD_MS);
					tryPic--;
			}
			
			if(jpeg_data_ok == 1)
			{
				unsigned char *p = (unsigned char *)dcmi_data_buff;
				int index = 0;
				while(p[0] != 0xff && p[1]!= 0xd8 && index < 100)
				{
					p++;
					index++;
				}
				
				
				memset(frameBuffer,0,100);/* ������� */
				sendTcpDataLen = jpeg_data_len * 4 + 1 - index;
				frameBuffer[0] = sendTcpDataLen;
				frameBuffer[1] = sendTcpDataLen>>8;
				frameBuffer[2] = sendTcpDataLen>>16;
				frameBuffer[3] = sendTcpDataLen>>24;

				frameBuffer[4] = PICTURE_DATA;
                    
        HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
				
				
				HAL_UART_Transmit(&huart1,(const unsigned char *)p,jpeg_data_len * 4 - index,0xFFFFFFFF);
				memset(dcmi_data_buff,0,PictureSize);/* ������� */
				
				memset(frameBuffer,0,100);/* ������� */
				sendTcpDataLen = 1;
				frameBuffer[0] = sendTcpDataLen;
				frameBuffer[1] = sendTcpDataLen>>8;
				frameBuffer[2] = sendTcpDataLen>>16;
				frameBuffer[3] = sendTcpDataLen>>24;

				frameBuffer[4] = CAMERA_TAKE_PICTURE_DONE;
                    
       HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
										
//				while(CDC_Transmit_HS((unsigned char *)dcmi_data_buff,jpeg_data_len * 4) != USBD_OK){};
				
				__HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, 0);
				jpeg_data_ok = 2;
			}
			takePictureFlag = 0;
		}
/*******************************************************************************�ⲿ��ƽ���**********************************************************************************************/
		static char keyFlag = 0;
		if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0 && keyFlag == 0)/* ���ϼ�⵽�͵�ƽ��ˮλ����ˮλ�� */
		{
				takePictureFlag = 1;
				keyFlag = 1;
//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:�ⲿ��������");
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_RESET);//��⵽ˮλ�������ر�ˮ��
//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:ˮ�ùر�");
//				vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		else if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1 && keyFlag == 1)/* �Ͽ���⵽�ߵ�ƽ */
		{
				keyFlag = 0;
		}
/*********************************************************************************�̵�������****************************************************************************************************/
		if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1) /* ˮλ��⿪��û�е���ˮλ�ߣ����Դ�ˮ�� */
		{
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_SET);//��ˮ�������ˮ��
				memset(frameBuffer,0,100);/* ������� */
				sendTcpDataLen = 1;
				frameBuffer[0] = sendTcpDataLen;
				frameBuffer[1] = sendTcpDataLen>>8;
				frameBuffer[2] = sendTcpDataLen>>16;
				frameBuffer[3] = sendTcpDataLen>>24;

				frameBuffer[4] = OPEN_MOTO_SUCCESS_CMD;
				HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
//				if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:3 TCP���ʹ���");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}

//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:ˮ�ô�");
				openMotoFlag = 0;
		}
		else if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0)
		{
//				ESP_LOGI("camera","ˮ�ô�ָ��ⲿ��⵽ˮλ�ѵ���");
				//��ʱ��ִֻ������
				takePictureFlag = 1;
				openMotoFlag = 0;
		}
						
		
		
    osDelay(1000);
  }
  /* USER CODE END StartSendDataTask */
}

/* USER CODE BEGIN Header_StartLedTask */
/**
* @brief Function implementing the ledTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLedTask */
void StartLedTask(void *argument)
{
  /* USER CODE BEGIN StartLedTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartLedTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/*
	vcpDataAnalysis(_RingBuffer *ringbuffer)
	����:����USB���⴮�ڽ��յ�����
*/
static void vcpDataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *RxFrameData;/*�洢����֡�Ŀռ�*/
	unsigned short FrameDataLen = ringbuffer->Lenght;
	if(xPortGetFreeHeapSize() >= FrameDataLen)
	{
		RxFrameData = (unsigned char *)pvPortMalloc(FrameDataLen);	/*Ϊ��ȡ�������е��������������ڴ�*/
		if(ReadBytes(ringbuffer,RxFrameData,FrameDataLen) == 1)		/*��ȡ�������е���������*/
		{
			/*��ȡ��������������*/
			for(int i=0;i<FrameDataLen;i++)	/*����*/
			{
				if(RxFrameData[i] == 0xAA)			/*��֡ͷ*/
				{
					if(FrameDataLen - i <= 3)		/*֡ͷ���������ݳ���*/
					{
						WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*δ���������д�ػ�����*/
						vPortFree(RxFrameData);  //�ͷ��ڴ�
						return ;/*���ؿ�����*/
					}
					
					unsigned short FrameLen = RxFrameData[i+3];			/*�õ�֡�������ݳ���*/
					
					if((FrameDataLen - i) >= (4 + FrameLen + 2))	/*ʣ�������Ƿ���һ֡�ĳ���*/
					{
						if(FrameDataCheck( &RxFrameData[i] ) == 1)		/*У��֡����*/
						{
							unsigned short OneFrameDataLen = 4 + FrameLen + 2;			/*����֡�ܳ�*/
							if(RxFrameData[i+1] == PC)														/*����λ�����͵�����*/
							{
								switch (RxFrameData[i+2])
								{
										case CDM_WIFI_NAME:
										{
//																					memset(wifiName,0,sizeof(wifiName));
//																					memcpy((char *)wifiName, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_WIFI_PASSWORD:
										{
//																					memset(wifiPassword,0,sizeof(wifiPassword));
//																					memcpy((char *)wifiPassword, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_SERVER_IP:
										{
												memset(deviceAttributeInfo.UDP_serverIP,0,sizeof(deviceAttributeInfo.UDP_serverIP));
												memcpy((char *)deviceAttributeInfo.UDP_serverIP, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_DEVICE_ID:
										{
												memset(deviceAttributeInfo.deviceID,0,sizeof(deviceAttributeInfo.deviceID));
												memcpy((char *)deviceAttributeInfo.deviceID, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_SERVER_PORT:
										{
												char temp[10];
												memcpy((char *)temp, &RxFrameData[i+4],FrameLen);
												deviceAttributeInfo.UDP_serverPort = atoi(temp);
												break;
										}
										case CMD_PICTURE_SIZE:
										{
//																					camera_config.frame_size = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_PICTURE_QUALITY:
										{
//																					camera_config.jpeg_quality = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_SET_LIED_BRIGHTNESS:
										{
												deviceAttributeInfo.ledFlashBrightness = RxFrameData[i+4] | RxFrameData[i+5] <<8;
//																					ESP_LOGI("UART","�Ƶ�����Ϊ = %d",deviceAttributeInfo.ledFlashBrightness);
												break;
										}
										case CMD_SET_TAKE_PICTURE_DELAY_TIME:
										{
												deviceAttributeInfo.takePictureDelayTime = RxFrameData[i+4] | RxFrameData[i+5] <<8;
//																					ESP_LOGI("UART","������ʱ = %d",deviceAttributeInfo.takePictureDelayTime);
												break;
										}

										case CMD_SET_PARA_END:
										{
											WriteFlashData(ADDR_FLASH_SECTOR_7,(unsigned char *)deviceAttributeInfo.picFormat,sizeof(_deviceInfo));
//																					writeDeviceInfo();
//																					espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:����д�����,2s������");
//																					ESP_LOGI("UART","����д�����,2s������");
//																					vTaskDelay(2000/portTICK_PERIOD_MS);
//																					esp_restart();/* �����豸 */
												break;
										}
										case CMD_SHOW_DEVICE_INFO:
										{
//																					showEspInfo();
												break;
										}
										case CMD_RESET_DEVICE:
										{
//																					esp_restart();
												break;
										}
										case CMD_SET_RECORD_TIME:
										{
												if(deviceAttributeInfo.recordTimeIndex > 50)
												{
//																							espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:����������ö�ʱ���飬��ɾ������");
														break;
												}
												memset((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],0,6);
												memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex], &RxFrameData[i+4],FrameLen);
												deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
												deviceAttributeInfo.recordTimeIndex++;
												deviceAttributeInfo.scheduledDeletion = 0;
												break;
										}
										case CMD_SET_DELAY_TIME:
										{
												deviceAttributeInfo.scheduledDeletion = RxFrameData[i+4] | RxFrameData[i+5] <<8;
//																					ESP_LOGI("info","scheduledDeletion = %d",deviceAttributeInfo.scheduledDeletion);
												for(int i=0;i<50;i++)/* �����ʱʱ����̶���ʱ���õ�ʱ�� */
												{
														memset(deviceAttributeInfo.recordTime[i],0,6);
												}
										}
										default:
												break;
								}
							}
							i += OneFrameDataLen-1;										/*ƫ��*/
						}
					}
					else
					{
						WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*δ���������д�ػ�����*/
						vPortFree(RxFrameData);  //�ͷ��ڴ�
						return ;
					}
				}
			}
		}
		vPortFree(RxFrameData);  //�ͷ��ڴ�
	}
}
/*
	usart1DataAnalysis(_RingBuffer *ringbuffer)
	����:����USART1���ڽ��յ�����
*/
static void usart1DataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *FrameData;                                               /*�洢����֡�Ŀռ�*/
	unsigned short allFrameDataLen = ringbuffer->Lenght;               			/* ��ȡ�������еĴ洢�����ݳ��� */
	int dataIndex = 0;                                                      /* �����е�ƫ��λ�� */

	if(xPortGetFreeHeapSize() >= allFrameDataLen)                           /* �ж�ʣ���ڴ��Ƿ���������ڴ� */
	{
			FrameData = (unsigned char *)pvPortMalloc(allFrameDataLen);	        /*Ϊ��ȡ�������е��������������ڴ�*/
			if(ReadBytes(ringbuffer,FrameData,allFrameDataLen) == 1)		/*��ȡ�������е���������*/
			{
					while(allFrameDataLen - dataIndex >= 4)/* ��Ȼ���Եõ����ݳ��� */
					{
							unsigned int frameDataLen = (int)FrameData[dataIndex + 0] |
																					(int)FrameData[dataIndex + 1] <<  8 |
																					(int)FrameData[dataIndex + 2] << 16 |
																					(int)FrameData[dataIndex + 3] << 24;/* �õ��������� */

							if(allFrameDataLen - dataIndex - 4 >= frameDataLen)/* �ܵõ��������� */
							{   
									dataIndex += 4;
									switch(FrameData[dataIndex])
									{
											case CAMERA_TAKE_PICTURE:/* ���� */
											{
//												CDC_Transmit_HS("ok\r\n",4);//USART1��������ת����VCP����
															takePictureFlag = 1;
//															espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:�յ�����ָ��");
													break;
											}
											case OPEN_MOTO_CMD:
											{
															openMotoFlag = 1;
//															espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:�յ���ˮ��ָ��");
													break;
											}
											case SET_RECORD_TIME_CMD:/* ���ù̶���ʱ */
											{
													dataIndex++;
													if(deviceAttributeInfo.recordTimeIndex > 50)
													{
//																	espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:����������ö�ʱ���飬��ɾ������");
															break;
													}
													memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],
																			&FrameData[dataIndex],5);
													deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
													
//															ESP_LOGI("TCP","�յ�����:%s",(char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex]);
													deviceAttributeInfo.recordTimeIndex++;
													
													deviceAttributeInfo.scheduledDeletion = 0;/* �̶���ʱʱ�������ʱʱ��������Ч */
													dataIndex--;
													// espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:��ʱʱ��");
													break;
											}
											case SET_RECORD_TIME_DONE_CMD:
											{
//															ESP_LOGI("TCP","��ʱ���ý���");
												deviceAttributeInfo.recordTimeIndex = 0;
												
//															writeDeviceInfo();/* д�����ݵ�NVS�б��� */
													break;
											}
											case SET_SCHEDULED_TIME_CMD:
											{
//															ESP_LOGI("TCP","�����ʱ");
													deviceAttributeInfo.scheduledDeletion = (unsigned short)FrameData[dataIndex + 1] | 
																																	(unsigned short)FrameData[dataIndex + 2]<<8;

													for(int i=0;i<50;i++)/* �����ʱʱ����̶���ʱ���õ�ʱ�� */
													{
															memset(deviceAttributeInfo.recordTime[i],0,6);
													}
//															ESP_LOGI("TCP","scheduledDeletion = %d",deviceAttributeInfo.scheduledDeletion);
													break;
											}
											case SET_LIED_BRIGHTNESS_CMD:   /* ������������� */
											{
//															ESP_LOGI("TCP","�������������");
													deviceAttributeInfo.ledFlashBrightness = (unsigned short)FrameData[dataIndex + 1] | 
																																	(unsigned short)FrameData[dataIndex + 2]<<8;

//															ESP_LOGI("TCP","ledFlashBrightness = %d",deviceAttributeInfo.ledFlashBrightness);
													break;
											}
											case SET_TAKE_PICTURE_DELAY_TIME_CMD:    /* ����������ʱʱ�� */
											{
//															ESP_LOGI("TCP","������ʱʱ��");
													deviceAttributeInfo.takePictureDelayTime = (unsigned short)FrameData[dataIndex + 1] | 
																																	(unsigned short)FrameData[dataIndex + 2]<<8;

//															ESP_LOGI("TCP","takePictureDelayTime = %d",deviceAttributeInfo.takePictureDelayTime);
													break;
											}
											default:
											{
													break;
											}
									}
									dataIndex += frameDataLen;
							}
							else/* �����ݳ��ȵ���û����������,������������,��һ�ν��� */
							{
									WriteBytes(ringbuffer,&FrameData[dataIndex],allFrameDataLen - dataIndex);/* д��ʣ������ */
									break;
							}
					}
			}
			vPortFree(FrameData);  //�ͷ��ڴ�
	}
}
/* USER CODE END Application */

