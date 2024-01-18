/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dcmi.h"
#include "usbd_cdc_if.h"
#include "main.h"
#include "usart.h"
#include "ringBuffer.h"
#include "usbd_cdc_if.h"
#include "flash.h"
#include "tim.h"
#include "air724.h"
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
char air724InitOk = 1;
char getTackPictureFlag = 0;
char takePictureFlag = 0;
char openMotoFlag = 0;
uint32_t lastSec = 0;
char anlysis = 0;
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
  .stack_size = 256 * 4,
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
  .priority = (osPriority_t) osPriorityBelowNormal7,
};
/* Definitions for motoTask */
osThreadId_t motoTaskHandle;
const osThreadAttr_t motoTask_attributes = {
  .name = "motoTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void showDeviceInfo(void);
static void vcpDataAnalysis(_RingBuffer *ringbuffer);
static void usart1DataAnalysis(_RingBuffer *ringbuffer);
static uint8_t sendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data);
static void frameDataAnalysis(uint8_t *data,uint32_t dataLen);
static void pcToCameraDeal(uint8_t *data);
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartCmdAnalysis(void *argument);
void StartSendDataTask(void *argument);
void StartLedTask(void *argument);
void StartMotoTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

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

  /* creation of motoTask */
  motoTaskHandle = osThreadNew(StartMotoTask, NULL, &motoTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
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
	uint16_t reTryCnt = 0;
	while(AIR724_INIT_OK != air724Init() && reTryCnt<10)
	{
		deviceAttributeInfo.sysState = AIR724_ERROR_STATE;
		reTryCnt++;
		HAL_Delay(5000);
	}
	if(10 == reTryCnt)
	{
		deviceAttributeInfo.sysState = AIR724_ERROR_STATE;
		air724InitOk = 1;
	}
	else
	{
		deviceAttributeInfo.sysState = SYS_OK_STATE;
		air724InitOk = AIR724_INIT_OK;
	}
	uint8_t *dataPtr = NULL;
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
		if(1 == sendRgbPicData)
		{
			jpeg_data_len = 960000;
			
			char dataLenBuffer[4];
			dataLenBuffer[0] = jpeg_data_len;
			dataLenBuffer[1] = jpeg_data_len >> 8;
			dataLenBuffer[2] = jpeg_data_len >> 16;
			dataLenBuffer[3] = jpeg_data_len >> 24;
			while(CDC_Transmit_HS((unsigned char *)dataLenBuffer,4) != USBD_OK){};
			HAL_Delay(100);
			/* 设置裁剪的感兴趣区域 */
			/* 总的尺寸是1200*1000,总的传输大小为1200*1000*2 = 2400000,单次传输1200*40,DMA大小为1200*40*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*30,DMA大小为1600*30*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*150,DMA大小为1600*150*2/4 =  240000*/
			for(int i=0;i<15;i++)
			{
				
				HAL_DCMI_DisableCrop(&hdcmi);
				HAL_Delay(1);
				HAL_DCMI_ConfigCrop (&hdcmi,0,80*i,1600*2-1,80-1);	
				HAL_Delay(1);
				HAL_DCMI_EnableCrop (&hdcmi);
				HAL_Delay(1);
				
				HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,64000);
				
				while(HAL_DMA_GetState(&hdma_dcmi)==HAL_DMA_STATE_BUSY){};
				HAL_DCMI_Stop(&hdcmi);
					
				dataPtr = (unsigned char *)dcmi_data_buff;

				while(CDC_Transmit_HS(dataPtr,64000) != USBD_OK){};
				dataPtr+=64000;
				HAL_Delay(50);
				while(CDC_Transmit_HS(dataPtr,64000) != USBD_OK){};
				dataPtr+=64000;
				HAL_Delay(50);
				while(CDC_Transmit_HS(dataPtr,64000) != USBD_OK){};
				dataPtr+=64000;
				HAL_Delay(50);
				while(CDC_Transmit_HS(dataPtr,64000) != USBD_OK){};
				HAL_Delay(50);
			}
			
			sendRgbPicData = 0;
		}
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartCmdAnalysis */
/**
* @brief Function implementing the cmdAnalysis thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCmdAnalysis */
void StartCmdAnalysis(void *argument)
{
  /* USER CODE BEGIN StartCmdAnalysis */
  /* Infinite loop */
  for(;;)
  {
		vcpDataAnalysis(&VCP_RxRingBuffer);
		if(anlysis == 1)
			usart1DataAnalysis(&UART_RxRingBuffer);
    osDelay(100);
  }
  /* USER CODE END StartCmdAnalysis */
}

/* USER CODE BEGIN Header_StartSendDataTask */
/**
* @brief Function implementing the sendDataTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSendDataTask */
void StartSendDataTask(void *argument)
{
  /* USER CODE BEGIN StartSendDataTask */
  	while(1 == air724InitOk)
	{
		osDelay(1000);
	}
  /* Infinite loop */
	uint8_t *dataPtr = NULL;
	lastSec = time.hour * 60 + time.minute;
	takePictureFlag = 0;
	
	uint8_t takePictureOverTime = 0;
	
  for(;;)
  {
    /*************************************************************************发送TCP心跳包**********************************************************************************************/
		static uint8_t heartBeatTimeCnt = 0;
		if(heartBeatTimeCnt == 200)
		{
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,HERAT_BEAT_PACK,0,0);
				heartBeatTimeCnt = 0;
		}
		heartBeatTimeCnt++;
		/*******************************************************************************固定定时判断**********************************************************************************************/		
		static unsigned char timeArrive = 0;
		char timeCmp[6];
		sprintf(timeCmp,"%02d:%02d",time.hour,time.minute);
		for(int i=0;i<50;i++)
		{
				if(strcmp(timeCmp,deviceAttributeInfo.recordTime[i]) == 0 && timeArrive != i)
				{
						openMotoFlag = 1;
						timeArrive = i;
				}
		}
		/*******************************************************************************间隔定时判断**********************************************************************************************/
		if(deviceAttributeInfo.scheduledDeletion != 0)/* 定时有效,单位分钟 */
		{
				uint32_t nowSec = time.hour * 60 + time.minute;
				if(nowSec < lastSec)	nowSec+=	24*60;
				if(nowSec - lastSec >= deviceAttributeInfo.scheduledDeletion)
				{
						lastSec = nowSec;
						openMotoFlag = 1;
				}
		}
		/*******************************************************************************拍照**********************************************************************************************/
		if(1 == getTackPictureFlag)
		{
			__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
			
			vTaskDelay(pdMS_TO_TICKS(deviceAttributeInfo.takePictureDelayTime*1000));
			
			/* 设置裁剪的感兴趣区域 */
			/* 总的尺寸是1200*1000,总的传输大小为1200*1000*2 = 2400000,单次传输1200*40,DMA大小为1200*40*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*30,DMA大小为1600*30*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*150,DMA大小为1600*150*2/4 =  240000*/
			for(int i=0;i<15;i++)
			{
				
				HAL_DCMI_DisableCrop(&hdcmi);
				HAL_Delay(1);
				HAL_DCMI_ConfigCrop (&hdcmi,0,80*i,1600*2-1,80-1);	
				HAL_Delay(1);
				HAL_DCMI_EnableCrop (&hdcmi);
				HAL_Delay(1);
				
				HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,64000);
				
				takePictureOverTime = 0;
				while(HAL_DMA_GetState(&hdma_dcmi)==HAL_DMA_STATE_BUSY && takePictureOverTime < 100)
				{
					takePictureOverTime++;
					HAL_Delay(10);
				}
				HAL_DCMI_Stop(&hdcmi);
					
				dataPtr = (unsigned char *)dcmi_data_buff;
				
				uint8_t temp[1];
				temp[0]	= i * 4;
					
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,64000,dataPtr);
			}
			
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,TAKE_RGB_PICTURE_END,0,0);
			
			__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
			
			getTackPictureFlag = 0;
		}
		if(takePictureFlag == 1)
		{
			__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
			
			vTaskDelay(pdMS_TO_TICKS(deviceAttributeInfo.takePictureDelayTime*1000));
			
			/* 设置裁剪的感兴趣区域 */
			/* 总的尺寸是1200*1000,总的传输大小为1200*1000*2 = 2400000,单次传输1200*40,DMA大小为1200*40*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*30,DMA大小为1600*30*2/4 =  24000*/
			/* 总的尺寸是1600*1200,总的传输大小为1600*1200*2 = 3840000,单次传输1600*150,DMA大小为1600*150*2/4 =  240000*/
			for(int i=0;i<15;i++)
			{
				
				HAL_DCMI_DisableCrop(&hdcmi);
				HAL_Delay(1);
				HAL_DCMI_ConfigCrop (&hdcmi,0,80*i,1600*2-1,80-1);	
				HAL_Delay(1);
				HAL_DCMI_EnableCrop (&hdcmi);
				HAL_Delay(1);
				
				HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,64000);
				
				takePictureOverTime = 0;
				while(HAL_DMA_GetState(&hdma_dcmi)==HAL_DMA_STATE_BUSY && takePictureOverTime < 100)
				{
					takePictureOverTime++;
					HAL_Delay(10);
				}
				HAL_DCMI_Stop(&hdcmi);
					
				dataPtr = (unsigned char *)dcmi_data_buff;
				
				uint8_t temp[1];
				temp[0]	= i * 4;
					
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,64000,dataPtr);
				dataPtr+=64000;
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,64000,dataPtr);
			}
			
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,TAKE_RGB_PICTURE_END,0,0);
			__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
			takePictureFlag = 0;
		}
    osDelay(10);
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

    osDelay(10);
  }
  /* USER CODE END StartLedTask */
}

/* USER CODE BEGIN Header_StartMotoTask */
/**
* @brief Function implementing the motoTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMotoTask */
void StartMotoTask(void *argument)
{
  /* USER CODE BEGIN StartMotoTask */
  /* Infinite loop */
  for(;;)
  {
		/*******************************************************************************外部电平检测**********************************************************************************************/
		static char keyFlag = 0;
		if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0 && keyFlag == 0)/* 吸合检测到低电平，水位到达水位线 */
		{
				takePictureFlag = 1;
				keyFlag = 1;
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_RESET);//检测到水位到达，立马关闭水泵
		}
		else if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1 && keyFlag == 1)/* 断开检测到高电平 */
		{
				keyFlag = 0;
		}
/*********************************************************************************继电器控制****************************************************************************************************/
		if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 1) /* 水位检测开关没有到达水位线，可以打开水泵 */
		{
				HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_SET);//打开水泵命令，打开水泵
				openMotoFlag = 0;
		}
		else if(openMotoFlag == 1 && HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0)
		{
				//此时就只执行拍照
				takePictureFlag = 1;
				openMotoFlag = 0;
		}
    osDelay(10);
  }
  /* USER CODE END StartMotoTask */
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
												memset(deviceAttributeInfo.wifiName,0,sizeof(deviceAttributeInfo.wifiName));
												memcpy((char *)deviceAttributeInfo.wifiName, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_WIFI_PASSWORD:
										{
												memset(deviceAttributeInfo.wifiPssword,0,sizeof(deviceAttributeInfo.wifiPssword));
												memcpy((char *)deviceAttributeInfo.wifiPssword, &RxFrameData[i+4],FrameLen);
												break;
										}
										case CMD_SERVER_IP:
										{
												memset(deviceAttributeInfo.serverIP,0,sizeof(deviceAttributeInfo.serverIP));
												memcpy((char *)deviceAttributeInfo.serverIP, &RxFrameData[i+4],FrameLen);
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
												deviceAttributeInfo.serverPort = atoi(temp);
												break;
										}
										case CMD_PICTURE_SIZE:
										{
												deviceAttributeInfo.picSize = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_PICTURE_QUALITY:
										{ 
												deviceAttributeInfo.jpegQuality = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_SET_LIED_BRIGHTNESS:
										{
												deviceAttributeInfo.ledFlashBrightness = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_SET_TAKE_PICTURE_DELAY_TIME:
										{
												deviceAttributeInfo.takePictureDelayTime = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_SET_PARA_END:
										{
												bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_6_BANK1);
												bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_6_BANK1,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
												deviceAttributeInfo.recordTimeIndex = 0;
												deviceAttributeInfo.isSetRecordTime = 1;
												break;
										}
										case CMD_SHOW_DEVICE_INFO:
										{
												showDeviceInfo();
												break;
										}
										case CMD_RESET_DEVICE:
										{
												break;
										}
										case CMD_SET_RECORD_TIME:
										{
												if(deviceAttributeInfo.recordTimeIndex > 50)
												{
														break;
												}
												if(deviceAttributeInfo.isSetRecordTime == 1)
												{
													for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
													{
															memset(deviceAttributeInfo.recordTime[i],0,6);
													}
													deviceAttributeInfo.isSetRecordTime = 0;
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
/* 显示设备设置的属性 */
static void showDeviceInfo(void)
{
	char logBuffer[100];
	sprintf(logBuffer,"\xE5\xBD\x93\xE5\x89\x8D\xE8\xAE\xBE\xE5\xA4\x87\xE4\xBF\xA1\xE6\x81\xAF\xE5\xA6\x82\xE4\xB8\x8B\xEF\xBC\x9A");/* 当前设备信息如下： */
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\x57\x49\x46\x49\xE5\x90\x8D\xE7\xA7\xB0\xE4\xB8\xBA\x3A%s",deviceAttributeInfo.wifiName);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\x57\x49\x46\x49\xE5\xAF\x86\xE7\xA0\x81\xE4\xB8\xBA\x3A%s",deviceAttributeInfo.wifiPssword);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8\xE5\x9C\xB0\xE5\x9D\x80\xE4\xB8\xBA\x3A%s",deviceAttributeInfo.serverIP);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8\xE7\xAB\xAF\xE5\x8F\xA3\xE4\xB8\xBA\x3A%04d",deviceAttributeInfo.serverPort);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE8\xAE\xBE\xE5\xA4\x87\x49\x44\xE4\xB8\xBA\x3A%s",deviceAttributeInfo.deviceID);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE9\x97\xAA\xE5\x85\x89\xE7\x81\xAF\xE4\xBA\xAE\xE5\xBA\xa6\xE4\xB8\xBA\x3A%04d",deviceAttributeInfo.ledFlashBrightness);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE6\x8B\x8D\xE7\x85\xA7\xE7\x9A\x84\xE5\xBB\xB6\xE6\x97\xB6\xE6\x97\xB6\xE9\x97\xB4\xE4\xB8\xBA\x3A%04d",deviceAttributeInfo.takePictureDelayTime);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE5\x9B\xBA\xE5\xAE\x9A\xE5\xAE\x9A\xE6\x97\xB6\xE7\xBB\x84\xE4\xB8\xBA\x3A");
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);

	for(uint32_t i=0;i<50;i++)
	{
		if(deviceAttributeInfo.recordTime[i][0] != 0)
		{
			memset(logBuffer,0,sizeof(logBuffer));
			sprintf(logBuffer,"%s",&deviceAttributeInfo.recordTime[i][0]);
			sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
		}
	}
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE9\x97\xB4\xE9\x9A\x94\xE5\xAE\x9A\xE6\x97\xB6\xE4\xB8\xBA\x3A%04d",deviceAttributeInfo.scheduledDeletion);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
}
static uint8_t sendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data)
{
    unsigned char frameDataLen = 0;
		uint32_t cnt = 0;
    char frameData[260];
    frameDataLen = strlen(data);
    frameData[cnt++] = frameHead;                               /* 帧头 */
    frameData[cnt++] = frameAddress;                            /* 帧地址 */
    frameData[cnt++] = frameID;                                 /* 帧ID */
    frameData[cnt++] = frameDataLen;                            /* 帧数据长度 */

		memcpy(&frameData[4],data,frameDataLen);
	
    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
		
		while(USBD_OK != CDC_Transmit_HS((uint8_t *)frameData,frameDataLen + 4 + 2)){};
    return frameDataLen + 4 + 2;
}

/*
	usart1DataAnalysis(_RingBuffer *ringbuffer)
	功能:解析USART1串口接收的数据
*/
static void usart1DataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *FrameData;                                               /* 存储解析帧的空间 */
	unsigned short allFrameDataLen = ringbuffer->Lenght;               			/* 获取缓冲区中的存储的数据长度 */                                                     
	uint32_t index = 0;																											/* 数组中的偏移位置 */
	
	if(xPortGetFreeHeapSize() >= allFrameDataLen)                           /* 判断剩余内存是否大于申请内存 */
	{
			FrameData = (unsigned char *)pvPortMalloc(allFrameDataLen);	        /* 为读取缓冲区中的所有数据申请内存 */
			if(ReadBytes(ringbuffer,FrameData,allFrameDataLen) == 1)						/* 读取缓冲区中的所有数据 */
			{
				for(uint32_t i=0;i<allFrameDataLen;i++)														/*  */		
				{
					if((0xAA == FrameData[i]) && (allFrameDataLen - i >= 8))				/* 确认数据帧头同时能得到数据长度 */
					{
						uint32_t dataLen = FrameData[i+4] | FrameData[i+5]<<8 | FrameData[i+6]<<16 | FrameData[i+7]<<24;
						if(allFrameDataLen - i >= 8+dataLen+1)                        /* 能都得到整个帧的数据长度 */
            {
                if(0xBB == FrameData[i+8+dataLen])                				/* 确认数据帧尾 */
                {
                    frameDataAnalysis(&FrameData[i],9+dataLen);      			/* 分析帧的数据 */
                }
                i = i + dataLen + 8;                                      /* 偏移 */
								index = i;
								break;
            }
					}
				}
				WriteBytes(ringbuffer,&FrameData[index],allFrameDataLen - index);
			}
			vPortFree(FrameData);  //释放内存
	}
}
static void frameDataAnalysis(uint8_t *data,uint32_t dataLen)
{
	switch(data[2])
	{
			case SERVER_TO_CLIENT:                      /* 服务器到pc */
			{
				break;
			}
			case PC_TO_CAMER:                           /* pc到camera */
			{
				pcToCameraDeal(data);
				break;
			}
			default:
				break;
	}
}
static void pcToCameraDeal(uint8_t *data)
{
	switch(data[3])
	{
		case TAKE_PICTURE:                           /* pc到camera */
		{
			getTackPictureFlag = 1;
			break;
		}
		case SET_RECORD_TIME:                           /* pc到camera */
		{
			uint32_t dataLen = data[4] | data[5]<<8 | data[6]<<16 | data[7]<<24;
			if(deviceAttributeInfo.recordTimeIndex > 50)
			{
					break;
			}
			if(deviceAttributeInfo.isSetRecordTime == 1)
			{
				for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
				{
						memset(deviceAttributeInfo.recordTime[i],0,6);
				}
				deviceAttributeInfo.isSetRecordTime = 0;
			}
			memset((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],0,6);
			memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex], &data[8],dataLen);
			deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
			deviceAttributeInfo.recordTimeIndex++;
			deviceAttributeInfo.scheduledDeletion = 0;
			break;
		}
		case SET_RECORD_TIME_END:                           /* pc到camera */
		{
			deviceAttributeInfo.recordTimeIndex = 0;
			deviceAttributeInfo.isSetRecordTime = 1;
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,SET_RECORD_TIME_SUCCESS,0,0);
			bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_6_BANK1);
			bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_6_BANK1,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
			break;
		}
		case SET_SCHEDULED_TIME:                           /* pc到camera */
		{
			deviceAttributeInfo.scheduledDeletion = data[8] | data[9] <<8;
			for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
			{
					memset(deviceAttributeInfo.recordTime[i],0,6);
			}
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,SET_SCHEDULED_TIME_CMD_SUCCESS,0,0);
			bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_6_BANK1);
			bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_6_BANK1,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
			break;
		}
		case OPEN_MOTO_CMD:
		{
			openMotoFlag = 1;
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,OPEN_MOTO_SUCCESS_CMD,0,0);
			break;
		}
		default:
			break;
	}
}
/* 给发送的TCP数据封帧，同时将数据推送出去 */
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data)
{
	/* 因为TCP数据包的数据量大，同时又因为单片机的内存岌岌可危，采用使用不同内存段来实现发送 */
	/* 0xAA byte1 byte2 byte3  byte4 byte5 byte6 byte7   byteN   0xBB */
	/* 帧头  角色   目标   作用       长度（四字节）          数据载荷   帧尾 */
	uint8_t tempData[20];
	uint32_t cnt = 0;
	tempData[cnt++] = 0xAA;                 /* 帧头 */
	tempData[cnt++] = charactar;      			/* 角色 */
	tempData[cnt++] = target;               /* 目标 */
	tempData[cnt++] = cmd;                  /* 作用 */
	tempData[cnt++] = dataLen;              /* 长度 */
	tempData[cnt++] = dataLen>>8;
	tempData[cnt++] = dataLen>>16;
	tempData[cnt++] = dataLen>>24;
	uint8_t reTryCnt = 0;
	while(HAL_OK != HAL_UART_Transmit(&huart1,(const uint8_t *)tempData,8,0xFFFFFFFF) && reTryCnt <5)
	{
		reTryCnt++;
	}
	if(0 != dataLen)
	{
		reTryCnt = 0;
		while(HAL_OK != HAL_UART_Transmit(&huart1,(const uint8_t *)data,dataLen,0xFFFFFFFF) && reTryCnt <5)
		{
			reTryCnt++;
		}
	}
	reTryCnt = 0;
	tempData[0] = 0xBB;
	while(HAL_OK != HAL_UART_Transmit(&huart1,(const uint8_t *)tempData,1,0xFFFFFFFF) && reTryCnt <5)
	{
		reTryCnt++;
	}
}


/* USER CODE END Application */

