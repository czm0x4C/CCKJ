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
#include "jpeg.h"  
#include "jpeg_utils.h"
#include "usbd_cdc_if.h"
#include "main.h"
#include "usart.h"
#include "ringBuffer.h"
#include "usbd_cdc_if.h"
#include "flash.h"
#include "tim.h"
#include "air724.h"
#include "ov5640.h"
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

uint8_t takePictureTestModeFlag = 0;

uint8_t serverIsOk = 0;
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
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal7,
};
/* Definitions for sendDataTask */
osThreadId_t sendDataTaskHandle;
const osThreadAttr_t sendDataTask_attributes = {
  .name = "sendDataTask",
  .stack_size = 512 * 4,
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
static uint8_t vcpSendData(uint8_t frameHead, uint8_t frameAddress, uint8_t frameID, uint8_t* data,uint8_t dataLen);
static uint8_t sendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data);
static void showDeviceInfo(void);
static void vcpDataAnalysis(_RingBuffer *ringbuffer);
static void usart1DataAnalysis(_RingBuffer *ringbuffer);

static void frameDataAnalysis(uint8_t *data,uint32_t dataLen);
static void serverToClient(uint8_t *data);
static void pcToCameraDeal(uint8_t *data);
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data);

static void takePictureOnTestMode(void);
static void takePictureCameraToPc(void);
static void takePictureCameraToServer(void);
static void skipFrame(void);
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
  /* Infinite loop */
  for(;;)
  {
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
	skipFrame();	/* 跳过最开始的几帧图像 */
  while(1 == air724InitOk)
	{
		osDelay(1000);
	}
  /* Infinite loop */
	/* 固定定时时间判断 */
	static unsigned char timeArrive = 0;
	char timeCmp[6];
	char logBuffer[100];
	lastSec = time.hour * 60 + time.minute;
	takePictureFlag = 0;
	
//	uint8_t takePictureOverTime = 0;
	
	JPEG_InitColorTables();
	
  for(;;)
  {
    /*************************************************************************发送TCP心跳包**********************************************************************************************/
		static uint8_t heartBeatTimeCnt = 0;		/* 不执行耗时的操作时，是10ms循环一次 */
		if(heartBeatTimeCnt == 200)
		{
				packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,HERAT_BEAT_PACK,0,0);
				heartBeatTimeCnt = 0;
		}
		heartBeatTimeCnt++;
		/*************************************************************************服务器检测**********************************************************************************************/
		static uint16_t serverTryCheckTimeCnt = 0;		/* 不执行耗时的操作时，是10ms循环一次 */
		uint8_t tryCnt = 0;
		if(serverTryCheckTimeCnt == 1000)
		{
				serverIsOk = 1;
				while(serverIsOk)
				{
					packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,CAMERA_TRY_SERVER_CHECK,0,0);
					if(tryCnt == 100)/* 2s内没有响应 */
					{
						NVIC_SystemReset(); 
					}
					tryCnt++;
					vTaskDelay(pdMS_TO_TICKS(20));
				}
				serverTryCheckTimeCnt = 0;
		}
		serverTryCheckTimeCnt++;
		/*************************************************************************测试模式**********************************************************************************************/
		if(SYS_TEST_MODE_SATE == deviceAttributeInfo.sysState)
		{
			sprintf(timeCmp,"%02d:%02d",time.hour,time.minute);
			for(int i=0;i<50;i++)
			{
					if(strcmp(timeCmp,deviceAttributeInfo.recordTime[i]) == 0 && timeArrive != i)
					{
							takePictureTestModeFlag = 1;
							timeArrive = i;
							sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xba\xe5\xae\x9a\xe5\xae\x9a\xe6\x97\xb6\xe9\x97\xb4\xe5\x88\xb0");/* 固定定时时间到 */
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
							takePictureFlag = 1;
							takePictureTestModeFlag = 1;
							sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe9\x97\xb4\xe9\x9a\x94\xe5\xae\x9a\xe6\x97\xb6\xe9\x97\xb4\xe5\x88\xb0");/* 间隔定时时间到 */
					}
			}
			if(1 == takePictureTestModeFlag)
			{
				takePictureOnTestMode();
				takePictureTestModeFlag = 0;
			}
		}
		/*************************************************************************正常工作模式**********************************************************************************************/
		else
		{
			/*******************************************************************************固定定时判断**********************************************************************************************/		
			
			sprintf(timeCmp,"%02d:%02d",time.hour,time.minute);
			for(int i=0;i<50;i++)
			{
					if(strcmp(timeCmp,deviceAttributeInfo.recordTime[i]) == 0 && timeArrive != i)
					{
							openMotoFlag = 1;
							takePictureFlag = 1;
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
							takePictureFlag = 1;
					}
			}
			/*******************************************************************************拍照**********************************************************************************************/
			if(1 == getTackPictureFlag)
			{
				takePictureCameraToPc();
				getTackPictureFlag = 0;
			}
			if(takePictureFlag == 1)
			{
				takePictureCameraToServer();
				takePictureFlag = 0;
			}
			/*******************************************************************************外部电平检测**********************************************************************************************/
			static char keyFlag = 0;
			if(HAL_GPIO_ReadPin(IN1_GPIO_Port,IN1_Pin) == 0 && keyFlag == 0)/* 吸合检测到低电平，水位到达水位线 */
			{
					HAL_GPIO_WritePin(MOTO_CTR_GPIO_Port, MOTO_CTR_Pin, GPIO_PIN_RESET);				//检测到水位到达，立马关闭水泵
					vTaskDelay(pdMS_TO_TICKS(deviceAttributeInfo.takePictureDelayTime*1000));		/* 延时等水箱水位静止 */
					takePictureFlag = 1;
					keyFlag = 1;
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
	char logBuffer[100];
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
												deviceAttributeInfo.serverPort = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												break;
										}
										case CMD_PICTURE_SIZE:
										{
												deviceAttributeInfo.picSize = RxFrameData[i+4] | RxFrameData[i+5] <<8;
												switch(deviceAttributeInfo.picSize)
												{
													case FRAMESIZE_QVGA:
													{
														deviceAttributeInfo.picWidth = 320;
														deviceAttributeInfo.picHeight = 240;
														break;
													}
													case FRAMESIZE_VGA:
													{
														deviceAttributeInfo.picWidth = 680;
														deviceAttributeInfo.picHeight = 480;
														break;
													}
													case FRAMESIZE_UXGA:
													{
														deviceAttributeInfo.picWidth = 1600;
														deviceAttributeInfo.picHeight = 1200;
														break;
													}
													case FRAMESIZE_QSXGA:
													{
														deviceAttributeInfo.picWidth = 2592;
														deviceAttributeInfo.picHeight = 1940;
														break;
													}
													default:
														break;
												}
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
										case CMD_SET_PARA_END:			/* 设置参数接收完成 */
										{
												bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
												bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(uint8_t *)deviceAttributeInfo.wifiName,sizeof(deviceAttributeInfo));
												deviceAttributeInfo.recordTimeIndex = 0;
												deviceAttributeInfo.isSetRecordTime = 1;
												
												sprintf(logBuffer,"\xE5\x8F\x82\xE6\x95\xB0\xE8\xAE\xBE\xE7\xBD\xAE\xE6\x88\x90\xE5\x8A\x9F\xEF\xBC\x81");/* 参数设置成功！ */
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
											
												break;
										}
										case CMD_SHOW_DEVICE_INFO:
										{
												showDeviceInfo();
												break;
										}
										case CMD_RESET_DEVICE:
										{
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe8\xae\xbe\xe5\xa4\x87\xe9\x87\x8d\xe5\x90\xaf");
												HAL_Delay(100);
												/* 系统复位 */
												NVIC_SystemReset(); 
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
												break;
										}
									  case CMD_SET_TEST_MODE:				/* 设备工作在测试模式 */
										{
												deviceAttributeInfo.sysState = SYS_TEST_MODE_SATE;
												sprintf(logBuffer,"\xE5\xB7\xB2\xE8\xAE\xBE\xE7\xBD\xAE\xE5\xB7\xA5\xE4\xBD\x9C\xE5\x9C\xA8\xE6\xB5\x8B\xE8\xAF\x95\xE6\xA8\xA1\xE5\xBC\x8F");/* 已设置工作在测试模式 */
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
												break;
										}
										case CMD_SET_WORK_MODE:				/* 设备工作在工作模式 */
										{
												deviceAttributeInfo.sysState = SYS_OK_STATE;
												sprintf(logBuffer,"\xE5\xB7\xB2\xE8\xAE\xBE\xE7\xBD\xAE\xE5\xB7\xA5\xE4\xBD\x9C\xE5\x9C\xA8\xE5\xB7\xA5\xE4\xBD\x9C\xE6\xA8\xA1\xE5\xBC\x8F");/* 已设置工作在工作模式 */
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
												break;
										}
										case CMD_TAKE_PICTURE_TEST_MODE: /* 设备拍照 */
										{
												takePictureTestModeFlag = 1;
												sprintf(logBuffer,"\xE6\x94\xB6\xE5\x88\xB0\xE6\x8B\x8D\xE7\x85\xA7\xE6\x8C\x87\xE4\xBB\xA4");/* 收到拍照指令 */
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
												break;
										}
										case CMD_OPEN_MOTO_TEST_MODE: /* 设备打开电机 */
										{
												sprintf(logBuffer,"\xE6\x94\xB6\xE5\x88\xB0\xE6\x89\x93\xE5\xBC\x80\xE7\x94\xB5\xE6\x9C\xBA\xE5\x91\xBD\xE4\xBB\xA4");/* 收到打开电机命令 */
												sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
												break;
										}
										case CMD_RUN_IAP:               /* 运行IAP程序 */
										{
											sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe8\xae\xbe\xe5\xa4\x87\xe9\x87\x8d\xe5\x90\xaf");
											
											bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
		
											deviceAttributeInfo.iapState = IAP_RUN_STATE;
											
											bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(uint8_t *)deviceAttributeInfo.wifiName,sizeof(deviceAttributeInfo));
											
											/* 系统复位 */
											 NVIC_SystemReset(); //复位
											break;
										}
										case CMD_SET_AUTO_FOCUS:
										{
												deviceAttributeInfo.automaticFocus =  RxFrameData[i+4] | RxFrameData[i+5] <<8;
												if(1 == deviceAttributeInfo.automaticFocus)
												{
													OV5640_Focus_Constant();
													OV5640_Focus_Init();
												}
												break;
										}
										case CMD_SET_CAMERA_PICTURE_DIRECTION:
										{
												deviceAttributeInfo.pictureDirection =  RxFrameData[i+4] | RxFrameData[i+5] <<8;
												switch(deviceAttributeInfo.pictureDirection)
												{
													case 0:
													{
														ov5640Normal();
														break;
													}
													case 1:
													{
														ov5640Mirror();
														break;
													}
													case 2:
													{
														ov5640Flip();
														break;
													}
													case 3:
													{
														ov5640MirrorAndFlip();
														break;
													}
													default:
														break;
												}
												break;
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

	/* 发送图片压缩质量 */
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xe5\x9b\xbe\xe5\x83\x8f\xe5\x8e\x8b\xe7\xbc\xa9\xe8\xb4\xa8\xe9\x87\x8f\x3A%04d",deviceAttributeInfo.jpegQuality);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	/* 发送图片尺寸 */
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xe5\x9b\xbe\xe5\x83\x8f\xe5\xb0\xba\xe5\xaf\xb8\x3A%04d\x2a%04d",deviceAttributeInfo.picWidth,deviceAttributeInfo.picHeight);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	/* JPEG压缩质量 */
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xe5\x9b\xbe\xe5\x83\x8f\xe5\x8e\x8b\xe7\xbc\xa9\xe8\xb4\xa8\xe9\x87\x8f\x3A%04d",deviceAttributeInfo.jpegQuality);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	/* 读取系统时间 */
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xe7\xb3\xbb\xe7\xbb\x9f\xe6\x97\xb6\xe9\x97\xb4\xe4\xb8\xba\x3a %02d\x3a%02d\x3a%02d",time.hour,time.minute,time.second);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	switch(deviceAttributeInfo.pictureDirection)
	{
		case 0:
		{
			sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xbe\xe7\x89\x87\xe6\x96\xb9\xe5\x90\x91\xe6\xad\xa3\xe5\xb8\xb8");	/* 图片方向正常 */
			break;
		}
		case 1:
		{
			sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xbe\xe7\x89\x87\xe6\x96\xb9\xe5\x90\x91\xe9\x95\x9c\xe5\x83\x8f");/* 图片方向镜像 */
			break;
		}
		case 2:
		{
			sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xbe\xe7\x89\x87\xe6\x96\xb9\xe5\x90\x91\xe7\xbf\xbb\xe8\xbd\xac");/* 图片方向翻转 */
			break;
		}
		case 3:
		{
			sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xbe\xe7\x89\x87\xe6\x96\xb9\xe5\x90\x91\xe9\x95\x9c\xe5\x83\x8f\xe7\xbf\xbb\xe8\xbd\xac");/* 图片方向镜像翻转 */
			break;
		}
		default:
			break;
	}
	/* 固定定时组 */
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
	
	/* 发送当前运行在APP程序的状态 */
	vcpSendData(0xAA,MCU,CMD_NOW_APP_STATE,0,0);
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
/* 虚拟串口发送数据 */
static uint8_t vcpSendData(uint8_t frameHead, uint8_t frameAddress, uint8_t frameID, uint8_t* data,uint8_t dataLen)
{
		uint32_t cnt = 0;
    uint8_t frameData[260];
	
    frameData[cnt++] = frameHead;                               /* 帧头 */
    frameData[cnt++] = frameAddress;                            /* 帧地址 */
    frameData[cnt++] = frameID;                                 /* 帧ID */
    frameData[cnt++] = dataLen;                            /* 帧数据长度 */

		memcpy(&frameData[4],data,dataLen);
	
    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[dataLen + 4] = sumcheck;
    frameData[dataLen + 5] = addcheck;
		
		while(USBD_OK != CDC_Transmit_HS((uint8_t *)frameData,dataLen + 4 + 2)){};
    return dataLen + 4 + 2;
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
			case SERVER_TO_CLIENT:                      /* 服务器到摄像头 */
			{
				serverToClient(data);
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
static void serverToClient(uint8_t *data)
{
	switch(data[3])
	{
		case SERVER_CHECK_BACK:                           /* server到client */
		{
			serverIsOk = 0;
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
			bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
			bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
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
			bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
			bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(unsigned char *)deviceAttributeInfo.wifiName,sizeof(_deviceInfo));
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

static void takePictureOnTestMode(void)
{
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, deviceAttributeInfo.ledFlashBrightness);
	
	JPEG_ConfTypeDef    jpeg_info;
	JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;
	uint32_t MCU_TotalNb=0;
	uint32_t ConvertedDataCount=0;

	uint8_t *tempP = NULL;
	uint8_t *jpegStart = NULL;
	uint8_t *jpegEnd = NULL;
	uint32_t jpegLen = 0;
	
	/* 设置裁剪的感兴趣区域 */
	if(0 == deviceAttributeInfo.picWidth || 0 == deviceAttributeInfo.picHeight)
	{
		deviceAttributeInfo.picWidth = 2592;
		deviceAttributeInfo.picHeight = 1940;
	}
	
	uint32_t picStartX = 0;
	uint32_t picStartY = 0;
	
	picStartX = (2592 - deviceAttributeInfo.picWidth)/2;
	picStartY = (1940 - deviceAttributeInfo.picHeight)/2;
	
	for(uint8_t i=0;i<5;i++)
	{
		if(1 == deviceAttributeInfo.automaticFocus)	/* 自动对焦有效 */
		{
			OV5640_Focus_Single();
		}
	
		HAL_DCMI_DisableCrop(&hdcmi);
		HAL_Delay(1);
		HAL_DCMI_ConfigCrop (&hdcmi,picStartX,picStartY,deviceAttributeInfo.picWidth*2-1,deviceAttributeInfo.picHeight-1);	
		HAL_Delay(1);
		HAL_DCMI_EnableCrop (&hdcmi);
		HAL_Delay(1);
		
		HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight / 2);
		while ((DCMI->CR & DCMI_CR_CAPTURE) != 0) {};
		HAL_DCMI_Stop(&hdcmi);
	}
	
		
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
		
	jpeg_info.ImageWidth = deviceAttributeInfo.picWidth;
	jpeg_info.ImageHeight = deviceAttributeInfo.picHeight;
	jpeg_info.ChromaSubsampling = JPEG_420_SUBSAMPLING;
	jpeg_info.ColorSpace = JPEG_YCBCR_COLORSPACE;
	jpeg_info.ImageQuality = deviceAttributeInfo.jpegQuality;
	HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_info);
		
	//根据编码信息，获取编码函数和数量
	JPEG_GetEncodeColorConvertFunc(&jpeg_info, &pRGBToYCbCr_Convert_Function, &MCU_TotalNb);
			
	//预处理，将RGB转换为指定的YUV格式
	pRGBToYCbCr_Convert_Function((uint8_t *)dcmi_data_buff, (uint8_t *)jpgDataBuffer, 0, deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight * 2,&ConvertedDataCount);
		
	HAL_JPEG_Encode(&hjpeg,(uint8_t *)jpgDataBuffer,ConvertedDataCount,(uint8_t *)dcmi_data_buff,JPG_DATA_BUFFER_SIZE,HAL_MAX_DELAY);
		
	tempP = (unsigned char *)dcmi_data_buff;
	jpegLen = 0;
	jpegStart = NULL;
	jpegEnd = NULL;
	for(int i=0;i<JPG_DATA_BUFFER_SIZE;)
	{
		if(0xFF == tempP[i] && 0xD8 == tempP[i+1])
		{
			jpegStart = &tempP[i];
		}
		if(0xFF == tempP[i] && 0xD9 == tempP[i+1])
		{
			jpegEnd = &tempP[i+1];
			break;
		}
		i++;
	}
	if(NULL != jpegStart && NULL != jpegEnd)								/* 判断数据检测是否有效 */
	{
		jpegLen = jpegEnd - jpegStart + 1;
	}
	
	if(jpegLen)																							/* 数据有效 */
	{
		uint8_t tempVcpSendDataBuffer[260];
		tempVcpSendDataBuffer[0] = jpegLen;
		tempVcpSendDataBuffer[1] = jpegLen >> 8;
		tempVcpSendDataBuffer[2] = jpegLen >> 16;
		tempVcpSendDataBuffer[3] = jpegLen >> 24;
		vcpSendData(0xAA,MCU,CMD_JPEG_DATA_PACK_SIZE,tempVcpSendDataBuffer,4);			/* 发送图片大小 */
		
		
		uint32_t clcTimes = 0;
		clcTimes = jpegLen / 240;
		for(int i=0;i<clcTimes;i++)
		{
			vcpSendData(0xAA,MCU,CMD_JPEG_DATA_PACK,jpegStart,240);
			jpegStart += 240;
		}
		jpegLen = jpegLen % 240;
		
		vcpSendData(0xAA,MCU,CMD_JPEG_DATA_PACK,jpegStart,jpegLen);
		vcpSendData(0xAA,MCU,CMD_JPEG_DATA_SEND_END,0,0);														/* 告知发送结束 */
	}
}
static void takePictureCameraToPc(void)
{
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, deviceAttributeInfo.ledFlashBrightness);
	
	JPEG_ConfTypeDef    jpeg_info;
	JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;
	uint32_t MCU_TotalNb=0;
	uint32_t ConvertedDataCount=0;

	uint8_t *tempP = NULL;
	uint8_t *jpegStart = NULL;
	uint8_t *jpegEnd = NULL;
	uint32_t jpegLen = 0;
	
	/* 设置裁剪的感兴趣区域 */
	if(0 == deviceAttributeInfo.picWidth || 0 == deviceAttributeInfo.picHeight)
	{
		deviceAttributeInfo.picWidth = 2592;
		deviceAttributeInfo.picHeight = 1940;
	}
	
	uint32_t picStartX = 0;
	uint32_t picStartY = 0;
	
	picStartX = (2592 - deviceAttributeInfo.picWidth)/2;
	picStartY = (1940 - deviceAttributeInfo.picHeight)/2;
	
	for(uint8_t i=0;i<5;i++)
	{
		if(1 == deviceAttributeInfo.automaticFocus)	/* 自动对焦有效 */
		{
			OV5640_Focus_Single();
		}
	
		HAL_DCMI_DisableCrop(&hdcmi);
		HAL_Delay(1);
		HAL_DCMI_ConfigCrop (&hdcmi,picStartX,picStartY,deviceAttributeInfo.picWidth*2-1,deviceAttributeInfo.picHeight-1);	
		HAL_Delay(1);
		HAL_DCMI_EnableCrop (&hdcmi);
		HAL_Delay(1);
		
		HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight / 2);
		while ((DCMI->CR & DCMI_CR_CAPTURE) != 0) {};
		HAL_DCMI_Stop(&hdcmi);
	}
	
		
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
		
	jpeg_info.ImageWidth = deviceAttributeInfo.picWidth;
	jpeg_info.ImageHeight = deviceAttributeInfo.picHeight;
	jpeg_info.ChromaSubsampling = JPEG_420_SUBSAMPLING;
	jpeg_info.ColorSpace = JPEG_YCBCR_COLORSPACE;
	jpeg_info.ImageQuality = deviceAttributeInfo.jpegQuality;
	HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_info);
		
	//根据编码信息，获取编码函数和数量
	JPEG_GetEncodeColorConvertFunc(&jpeg_info, &pRGBToYCbCr_Convert_Function, &MCU_TotalNb);
			
	//预处理，将RGB转换为指定的YUV格式
	pRGBToYCbCr_Convert_Function((uint8_t *)dcmi_data_buff, (uint8_t *)jpgDataBuffer, 0, deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight * 2,&ConvertedDataCount);
		
	HAL_JPEG_Encode(&hjpeg,(uint8_t *)jpgDataBuffer,ConvertedDataCount,(uint8_t *)dcmi_data_buff,JPG_DATA_BUFFER_SIZE,HAL_MAX_DELAY);
		
	tempP = (unsigned char *)dcmi_data_buff;
	jpegLen = 0;
	jpegStart = NULL;
	jpegEnd = NULL;
	for(int i=0;i<JPG_DATA_BUFFER_SIZE;)
	{
		if(0xFF == tempP[i] && 0xD8 == tempP[i+1])
		{
			jpegStart = &tempP[i];
		}
		if(0xFF == tempP[i] && 0xD9 == tempP[i+1])
		{
			jpegEnd = &tempP[i+1];
			break;
		}
		i++;
	}
	if(NULL != jpegStart && NULL != jpegEnd)								/* 判断数据检测是否有效 */
	{
		jpegLen = jpegEnd - jpegStart + 1;
	}
	
	if(jpegLen)																							/* 数据有效 */
	{
		uint8_t jpegDataLen[4];
		jpegDataLen[0] = jpegLen;
		jpegDataLen[1] = jpegLen >> 8;
		jpegDataLen[2] = jpegLen >> 16;
		jpegDataLen[3] = jpegLen >> 24;
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,JPEG_DATA_PACK_SIZE,4,jpegDataLen);/* 发送图片大小 */
		
		
		uint32_t clcTimes = 0;
		clcTimes = jpegLen / 64000;
		for(int i=0;i<clcTimes;i++)
		{
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,64000,jpegStart);
			jpegStart += 64000;
		}
		jpegLen = jpegLen % 64000;
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,PICTURE_DATA_PACK,jpegLen,jpegStart);
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_PC,TAKE_PICTURE_END,0,0);/* 告知发送结束 */
	}
}

static void takePictureCameraToServer(void)
{
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, deviceAttributeInfo.ledFlashBrightness);
	
	JPEG_ConfTypeDef    jpeg_info;
	JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;
	uint32_t MCU_TotalNb=0;
	uint32_t ConvertedDataCount=0;

	uint8_t *tempP = NULL;
	uint8_t *jpegStart = NULL;
	uint8_t *jpegEnd = NULL;
	uint32_t jpegLen = 0;
	
	/* 设置裁剪的感兴趣区域 */
	if(0 == deviceAttributeInfo.picWidth || 0 == deviceAttributeInfo.picHeight)
	{
		deviceAttributeInfo.picWidth = 2592;
		deviceAttributeInfo.picHeight = 1940;
	}
	
	uint32_t picStartX = 0;
	uint32_t picStartY = 0;
	
	picStartX = (2592 - deviceAttributeInfo.picWidth)/2;
	picStartY = (1940 - deviceAttributeInfo.picHeight)/2;
	
	for(uint8_t i=0;i<5;i++)
	{
		if(1 == deviceAttributeInfo.automaticFocus)	/* 自动对焦有效 */
		{
			OV5640_Focus_Single();
		}
	
		HAL_DCMI_DisableCrop(&hdcmi);
		HAL_Delay(1);
		HAL_DCMI_ConfigCrop (&hdcmi,picStartX,picStartY,deviceAttributeInfo.picWidth*2-1,deviceAttributeInfo.picHeight-1);	
		HAL_Delay(1);
		HAL_DCMI_EnableCrop (&hdcmi);
		HAL_Delay(1);
		
		HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight / 2);
		while ((DCMI->CR & DCMI_CR_CAPTURE) != 0) {};
		HAL_DCMI_Stop(&hdcmi);
	}
	
		
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
		
	jpeg_info.ImageWidth = deviceAttributeInfo.picWidth;
	jpeg_info.ImageHeight = deviceAttributeInfo.picHeight;
	jpeg_info.ChromaSubsampling = JPEG_420_SUBSAMPLING;
	jpeg_info.ColorSpace = JPEG_YCBCR_COLORSPACE;
	jpeg_info.ImageQuality = deviceAttributeInfo.jpegQuality;
	HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_info);
		
	//根据编码信息，获取编码函数和数量
	JPEG_GetEncodeColorConvertFunc(&jpeg_info, &pRGBToYCbCr_Convert_Function, &MCU_TotalNb);
			
	//预处理，将RGB转换为指定的YUV格式
	pRGBToYCbCr_Convert_Function((uint8_t *)dcmi_data_buff, (uint8_t *)jpgDataBuffer, 0, deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight * 2,&ConvertedDataCount);
		
	HAL_JPEG_Encode(&hjpeg,(uint8_t *)jpgDataBuffer,ConvertedDataCount,(uint8_t *)dcmi_data_buff,JPG_DATA_BUFFER_SIZE,HAL_MAX_DELAY);
		
	tempP = (unsigned char *)dcmi_data_buff;
	jpegLen = 0;
	jpegStart = NULL;
	jpegEnd = NULL;
	for(int i=0;i<JPG_DATA_BUFFER_SIZE;)
	{
		if(0xFF == tempP[i] && 0xD8 == tempP[i+1])
		{
			jpegStart = &tempP[i];
		}
		if(0xFF == tempP[i] && 0xD9 == tempP[i+1])
		{
			jpegEnd = &tempP[i+1];
			break;
		}
		i++;
	}
	if(NULL != jpegStart && NULL != jpegEnd)								/* 判断数据检测是否有效 */
	{
		jpegLen = jpegEnd - jpegStart + 1;
	}
	
	if(jpegLen)																							/* 数据有效 */
	{
		uint8_t jpegDataLen[4];
		jpegDataLen[0] = jpegLen;
		jpegDataLen[1] = jpegLen >> 8;
		jpegDataLen[2] = jpegLen >> 16;
		jpegDataLen[3] = jpegLen >> 24;
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,JPEG_DATA_PACK_SIZE,4,jpegDataLen);/* 发送图片大小 */
		
		
		uint32_t clcTimes = 0;
		clcTimes = jpegLen / 64000;
		for(int i=0;i<clcTimes;i++)
		{
			packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,64000,jpegStart);
			jpegStart += 64000;
		}
		jpegLen = jpegLen % 64000;
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,PICTURE_DATA_PACK,jpegLen,jpegStart);
		
		packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,TAKE_PICTURE_END,0,0);/* 告知发送结束 */
	}
}
static void skipFrame(void)
{
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, deviceAttributeInfo.ledFlashBrightness);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, deviceAttributeInfo.ledFlashBrightness);
	
	/* 设置裁剪的感兴趣区域 */
	if(0 == deviceAttributeInfo.picWidth || 0 == deviceAttributeInfo.picHeight)
	{
		deviceAttributeInfo.picWidth = 2592;
		deviceAttributeInfo.picHeight = 1940;
	}
	
	uint32_t picStartX = 0;
	uint32_t picStartY = 0;
	
	picStartX = (2592 - deviceAttributeInfo.picWidth)/2;
	picStartY = (1940 - deviceAttributeInfo.picHeight)/2;
	
	for(uint8_t i=0;i<5;i++)
	{
		if(1 == deviceAttributeInfo.automaticFocus)	/* 自动对焦有效 */
		{
			OV5640_Focus_Single();
		}
	
		HAL_DCMI_DisableCrop(&hdcmi);
		HAL_Delay(1);
		HAL_DCMI_ConfigCrop (&hdcmi,picStartX,picStartY,deviceAttributeInfo.picWidth*2-1,deviceAttributeInfo.picHeight-1);	
		HAL_Delay(1);
		HAL_DCMI_EnableCrop (&hdcmi);
		HAL_Delay(1);
		
		HAL_DCMI_Start_DMA(&hdcmi,DCMI_MODE_SNAPSHOT,(uint32_t)&dcmi_data_buff,deviceAttributeInfo.picWidth * deviceAttributeInfo.picHeight / 2);
		while ((DCMI->CR & DCMI_CR_CAPTURE) != 0) {};
		HAL_DCMI_Stop(&hdcmi);
	}
		
	__HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_4, 0);
	__HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, 0);
	
}
/* USER CODE END Application */

