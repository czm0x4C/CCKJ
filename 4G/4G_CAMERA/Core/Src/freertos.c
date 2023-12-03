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
	StartCmdAnalysis	解算接收的数据
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
	taskENTER_CRITICAL();//进入灵临界区
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
	taskEXIT_CRITICAL();//退出临界区
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
    vTaskDelayUntil(&PreviousWakeTime, configTICK_RATE_HZ/100);//指令解析任务执行周期为10Hz
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
	
	
	
	memset(frameBuffer,0,100);/* 清空数组 */
	sendTcpDataLen = 1;
	frameBuffer[0] = sendTcpDataLen;
	frameBuffer[1] = sendTcpDataLen>>8;
	frameBuffer[2] = sendTcpDataLen>>16;
	frameBuffer[3] = sendTcpDataLen>>24;
	frameBuffer[4] = SET_CAMERA_DEVICE_FLAG;
	HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
		
	memset(frameBuffer,0,100);/* 清空数组 */
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
		/* 发送TCP心跳包 */
		memset(frameBuffer,0,100);/* 清空数组 */
		sendTcpDataLen = 1;
		frameBuffer[0] = sendTcpDataLen;
		frameBuffer[1] = sendTcpDataLen>>8;
		frameBuffer[2] = sendTcpDataLen>>16;
		frameBuffer[3] = sendTcpDataLen>>24;

		frameBuffer[4] = HERAT_BEAT_PACK;
		HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
		
/*******************************************************************************拍照**********************************************************************************************/
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
				
				
				memset(frameBuffer,0,100);/* 清空数组 */
				sendTcpDataLen = jpeg_data_len * 4 + 1 - index;
				frameBuffer[0] = sendTcpDataLen;
				frameBuffer[1] = sendTcpDataLen>>8;
				frameBuffer[2] = sendTcpDataLen>>16;
				frameBuffer[3] = sendTcpDataLen>>24;

				frameBuffer[4] = PICTURE_DATA;
                    
        HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
				
				
				HAL_UART_Transmit(&huart1,(const unsigned char *)p,jpeg_data_len * 4 - index,0xFFFFFFFF);
				memset(dcmi_data_buff,0,PictureSize);/* 清空数组 */
				
				memset(frameBuffer,0,100);/* 清空数组 */
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
/*******************************************************************************外部电平检测**********************************************************************************************/
		static char keyFlag = 0;
		if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0 && keyFlag == 0)/* 吸合检测到低电平，水位到达水位线 */
		{
				takePictureFlag = 1;
				keyFlag = 1;
//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:外部触发拍照");
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_RESET);//检测到水位到达，立马关闭水泵
//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:水泵关闭");
//				vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		else if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1 && keyFlag == 1)/* 断开检测到高电平 */
		{
				keyFlag = 0;
		}
/*********************************************************************************继电器控制****************************************************************************************************/
		if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1) /* 水位检测开关没有到达水位线，可以打开水泵 */
		{
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_SET);//打开水泵命令，打开水泵
				memset(frameBuffer,0,100);/* 清空数组 */
				sendTcpDataLen = 1;
				frameBuffer[0] = sendTcpDataLen;
				frameBuffer[1] = sendTcpDataLen>>8;
				frameBuffer[2] = sendTcpDataLen>>16;
				frameBuffer[3] = sendTcpDataLen>>24;

				frameBuffer[4] = OPEN_MOTO_SUCCESS_CMD;
				HAL_UART_Transmit(&huart1,(const unsigned char *)frameBuffer,5,0xFFFFFFFF);
//				if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:3 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}

//				espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:水泵打开");
				openMotoFlag = 0;
		}
		else if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0)
		{
//				ESP_LOGI("camera","水泵打开指令，外部检测到水位已到达");
				//此时就只执行拍照
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
	功能:解析USB虚拟串口接收的数据
*/
static void vcpDataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *RxFrameData;/*存储解析帧的空间*/
	unsigned short FrameDataLen = ringbuffer->Lenght;
	if(xPortGetFreeHeapSize() >= FrameDataLen)
	{
		RxFrameData = (unsigned char *)pvPortMalloc(FrameDataLen);	/*为读取缓冲区中的所有数据申请内存*/
		if(ReadBytes(ringbuffer,RxFrameData,FrameDataLen) == 1)		/*读取缓冲区中的所有数据*/
		{
			/*读取到缓冲区的数据*/
			for(int i=0;i<FrameDataLen;i++)	/*遍历*/
			{
				if(RxFrameData[i] == 0xAA)			/*找帧头*/
				{
					if(FrameDataLen - i <= 3)		/*帧头后面无数据长度*/
					{
						WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
						vPortFree(RxFrameData);  //释放内存
						return ;/*返回空数据*/
					}
					
					unsigned short FrameLen = RxFrameData[i+3];			/*得到帧数据内容长度*/
					
					if((FrameDataLen - i) >= (4 + FrameLen + 2))	/*剩余数据是否还有一帧的长度*/
					{
						if(FrameDataCheck( &RxFrameData[i] ) == 1)		/*校验帧数据*/
						{
							unsigned short OneFrameDataLen = 4 + FrameLen + 2;			/*返回帧总长*/
							if(RxFrameData[i+1] == PC)														/*是上位机发送的数据*/
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
//																					ESP_LOGI("UART","灯的亮度为 = %d",deviceAttributeInfo.ledFlashBrightness);
												break;
										}
										case CMD_SET_TAKE_PICTURE_DELAY_TIME:
										{
												deviceAttributeInfo.takePictureDelayTime = RxFrameData[i+4] | RxFrameData[i+5] <<8;
//																					ESP_LOGI("UART","拍照延时 = %d",deviceAttributeInfo.takePictureDelayTime);
												break;
										}

										case CMD_SET_PARA_END:
										{
											WriteFlashData(ADDR_FLASH_SECTOR_7,(unsigned char *)deviceAttributeInfo.picFormat,sizeof(_deviceInfo));
//																					writeDeviceInfo();
//																					espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:数据写入完成,2s后重启");
//																					ESP_LOGI("UART","数据写入完成,2s后重启");
//																					vTaskDelay(2000/portTICK_PERIOD_MS);
//																					esp_restart();/* 重启设备 */
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
//																							espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:超过最大设置定时分组，请删除重试");
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
												for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
												{
														memset(deviceAttributeInfo.recordTime[i],0,6);
												}
										}
										default:
												break;
								}
							}
							i += OneFrameDataLen-1;										/*偏移*/
						}
					}
					else
					{
						WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
						vPortFree(RxFrameData);  //释放内存
						return ;
					}
				}
			}
		}
		vPortFree(RxFrameData);  //释放内存
	}
}
/*
	usart1DataAnalysis(_RingBuffer *ringbuffer)
	功能:解析USART1串口接收的数据
*/
static void usart1DataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *FrameData;                                               /*存储解析帧的空间*/
	unsigned short allFrameDataLen = ringbuffer->Lenght;               			/* 获取缓冲区中的存储的数据长度 */
	int dataIndex = 0;                                                      /* 数组中的偏移位置 */

	if(xPortGetFreeHeapSize() >= allFrameDataLen)                           /* 判断剩余内存是否大于申请内存 */
	{
			FrameData = (unsigned char *)pvPortMalloc(allFrameDataLen);	        /*为读取缓冲区中的所有数据申请内存*/
			if(ReadBytes(ringbuffer,FrameData,allFrameDataLen) == 1)		/*读取缓冲区中的所有数据*/
			{
					while(allFrameDataLen - dataIndex >= 4)/* 仍然可以得到数据长度 */
					{
							unsigned int frameDataLen = (int)FrameData[dataIndex + 0] |
																					(int)FrameData[dataIndex + 1] <<  8 |
																					(int)FrameData[dataIndex + 2] << 16 |
																					(int)FrameData[dataIndex + 3] << 24;/* 得到长度数据 */

							if(allFrameDataLen - dataIndex - 4 >= frameDataLen)/* 能得到数据内容 */
							{   
									dataIndex += 4;
									switch(FrameData[dataIndex])
									{
											case CAMERA_TAKE_PICTURE:/* 拍照 */
											{
//												CDC_Transmit_HS("ok\r\n",4);//USART1接收数据转发到VCP发送
															takePictureFlag = 1;
//															espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到拍照指令");
													break;
											}
											case OPEN_MOTO_CMD:
											{
															openMotoFlag = 1;
//															espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到打开水泵指令");
													break;
											}
											case SET_RECORD_TIME_CMD:/* 设置固定定时 */
											{
													dataIndex++;
													if(deviceAttributeInfo.recordTimeIndex > 50)
													{
//																	espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:超过最大设置定时分组，请删除重试");
															break;
													}
													memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],
																			&FrameData[dataIndex],5);
													deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
													
//															ESP_LOGI("TCP","收到数据:%s",(char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex]);
													deviceAttributeInfo.recordTimeIndex++;
													
													deviceAttributeInfo.scheduledDeletion = 0;/* 固定定时时，间隔定时时间设置无效 */
													dataIndex--;
													// espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:定时时间");
													break;
											}
											case SET_RECORD_TIME_DONE_CMD:
											{
//															ESP_LOGI("TCP","定时设置结束");
												deviceAttributeInfo.recordTimeIndex = 0;
												
//															writeDeviceInfo();/* 写入数据到NVS中保存 */
													break;
											}
											case SET_SCHEDULED_TIME_CMD:
											{
//															ESP_LOGI("TCP","间隔定时");
													deviceAttributeInfo.scheduledDeletion = (unsigned short)FrameData[dataIndex + 1] | 
																																	(unsigned short)FrameData[dataIndex + 2]<<8;

													for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
													{
															memset(deviceAttributeInfo.recordTime[i],0,6);
													}
//															ESP_LOGI("TCP","scheduledDeletion = %d",deviceAttributeInfo.scheduledDeletion);
													break;
											}
											case SET_LIED_BRIGHTNESS_CMD:   /* 设置闪光灯亮度 */
											{
//															ESP_LOGI("TCP","设置闪光灯亮度");
													deviceAttributeInfo.ledFlashBrightness = (unsigned short)FrameData[dataIndex + 1] | 
																																	(unsigned short)FrameData[dataIndex + 2]<<8;

//															ESP_LOGI("TCP","ledFlashBrightness = %d",deviceAttributeInfo.ledFlashBrightness);
													break;
											}
											case SET_TAKE_PICTURE_DELAY_TIME_CMD:    /* 设置拍照延时时间 */
											{
//															ESP_LOGI("TCP","拍照延时时间");
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
							else/* 有数据长度但是没有数据内容,继续接受数据,下一次解析 */
							{
									WriteBytes(ringbuffer,&FrameData[dataIndex],allFrameDataLen - dataIndex);/* 写入剩余数据 */
									break;
							}
					}
			}
			vPortFree(FrameData);  //释放内存
	}
}
/* USER CODE END Application */

