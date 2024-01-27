/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "flash.h"
#include "ringBuffer.h"
#include "usbd_cdc_if.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint8_t sendLogMessage(uint8_t frameHead, uint8_t frameAddress, uint8_t frameID, char* data);
static uint8_t vcpSendData(uint8_t frameHead, uint8_t frameAddress, uint8_t frameID, uint8_t* data,uint8_t dataLen);
static void showDeviceInfo(void);

typedef  void (*pFunction)(void);
static void JumpToApp(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
_deviceInfo deviceAttributeInfo;
uint8_t binFileDataBuffer[512 * 1024] __attribute__((at(0x24000000)));/* 存放bin文件的数组 */

uint32_t sysclock = 0;  
uint8_t binFileErrorFlag = 0;		/* 固件错误标志 */
uint32_t binFileSize = 0;				/* 固件大小 */
uint8_t binFileReceiveOK = 0;		/* 固件接收是否完成标志 */
uint8_t sendDeviceInfoFlag = 0;	/* 发送设备信息标志 */

pFunction JumpToApplication;
uint32_t JumpAddress;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	bsp_ReadCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(uint8_t *)&deviceAttributeInfo.wifiName[0],sizeof(deviceAttributeInfo));
	if(APP_RUN_STATE == deviceAttributeInfo.iapState)	/* 跳转APP运行 */
	{
		JumpToApp();
	}
	else if(IAP_RUN_STATE == deviceAttributeInfo.iapState)
	{
		deviceAttributeInfo.iapState = IAP_RUN_STATE;
	}
	else
	{
		memset((uint8_t *)deviceAttributeInfo.wifiName,0,sizeof(deviceAttributeInfo));
		bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
		
		deviceAttributeInfo.iapState = IAP_RUN_STATE;
		
		bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(uint8_t *)deviceAttributeInfo.wifiName,sizeof(deviceAttributeInfo));
		
		/* 系统复位 */
		 NVIC_SystemReset(); //复位
	}
	
		
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
	sysclock = HAL_RCC_GetSysClockFreq();
	
	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(1 == binFileReceiveOK)	/* 固件接收完成 */
		{
			/* 将固件写入指定的FLASH区域 */
			if(binFileSize < 128 * 1024)/* 固件大小小于1个小扇区的大小 */
			{
					bsp_EraseCpuFlash(APPLICATION_ADDRESS);
					uint8_t res = bsp_WriteCpuFlash(APPLICATION_ADDRESS,binFileDataBuffer,binFileSize);
					if(0 == res)
					{
						/* 固件写入FLASH成功 */
						sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x9b\xba\xe4\xbb\xb6\xe5\x86\x99\xe5\x85\xa5\x46\x4c\x41\x53\x48\xe6\x88\x90\xe5\x8a\x9f");
						
						bsp_EraseCpuFlash((uint32_t)ADDR_FLASH_SECTOR_7_BANK2);
		
						deviceAttributeInfo.iapState = APP_RUN_STATE;
						
						bsp_WriteCpuFlash(ADDR_FLASH_SECTOR_7_BANK2,(uint8_t *)deviceAttributeInfo.wifiName,sizeof(deviceAttributeInfo));
						
						/* 系统复位 */
						 NVIC_SystemReset(); //复位
					}
					else if(1 == res)
					{
						/* 数据长度或地址溢出 */
						sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe6\x95\xb0\xe6\x8d\xae\xe9\x95\xbf\xe5\xba\xa6\xe6\x88\x96\xe5\x9c\xb0\xe5\x9d\x80\xe6\xba\xa2\xe5\x87\xba");
					}
					else if(2 == res)
					{
						/* 写Flash出错 */
						sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x86\x99\x46\x6c\x61\x73\x68\xe5\x87\xba\xe9\x94\x99");
					}
			}
			binFileReceiveOK = 0;
		}
		if(1 == sendDeviceInfoFlag)
		{
			showDeviceInfo();
			sendDeviceInfoFlag = 0;
		}
		
		HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/*
	vcpDataAnalysis(_RingBuffer *ringbuffer)
	功能:解析USB虚拟串口接收的数据
*/
uint8_t tempDataBuffer[RINGBUFF_LEN];
char logBuffer[100];
void vcpDataAnalysis(_RingBuffer *ringbuffer)
{
	unsigned char *RxFrameData;/*存储解析帧的空间*/
	unsigned short FrameDataLen = ringbuffer->Lenght;
	static uint32_t binFilePackReceivedSize = 0;
	static uint8_t *receiveDataPtr = NULL;
	
	RxFrameData = tempDataBuffer;	/*为读取缓冲区中的所有数据申请内存*/
	if(ReadBytes(ringbuffer,RxFrameData,FrameDataLen) == 1)		/*读取缓冲区中的所有数据*/
	{
		/*读取到缓冲区的数据*/
		for(uint32_t i=0;i<FrameDataLen;i++)	/*遍历*/
		{
			if(RxFrameData[i] == 0xAA)			/*找帧头*/
			{
				if(FrameDataLen - i <= 3)		/*帧头后面无数据长度*/
				{
					WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
					break ;/*返回空数据*/
				}
				
				if(RxFrameData[i+1] != PC)
				{
					continue;
				}
				
				unsigned short FrameLen = RxFrameData[i+3];			/*得到帧数据内容长度*/
				
				if((FrameDataLen - i) >= (4 + FrameLen + 2))	/*剩余数据是否还有一帧的长度*/
				{
					if(FrameDataCheck( &RxFrameData[i] ) == 1)		/*校验帧数据*/
					{
						unsigned short OneFrameDataLen = 4 + FrameLen + 2;			/*返回帧总长*/

						switch (RxFrameData[i+2])
						{
								case CMD_SEND_BIN_FILE_SIZE:			/* 接收固件文件的大小 */
								{
										binFileSize =  RxFrameData[i+4] | RxFrameData[i+5] <<8 | RxFrameData[i+6] <<16 | RxFrameData[i+7] <<24;;
										sprintf(logBuffer,"\xE5\x9B\xBA\xE4\xBB\xB6\xE6\x96\x87\xE4\xBB\xB6\xE5\xA4\xA7\xE5\xB0\x8F\xE4\xB8\xBA\xEF\xBC\x9A%d\x62\x79\x74\x65",binFileSize);/* 固件文件大小为： byte */
										sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
										if(binFileSize > BIN_FILE_MAX_SIZE) /* 超过最大的固件接收大小 */
										{
											binFileErrorFlag = 1;
											break;
										}
										else
										{
											binFileErrorFlag = 0;
										}
										binFilePackReceivedSize = 0; 			/* 接收大小清零 */
										receiveDataPtr = &binFileDataBuffer[0];
										break;
								}
								case CMD_SEND_BIN_FILE_PACK:			/* 接收固件文件 */
								{
										if(1 == binFileErrorFlag)break;
										binFilePackReceivedSize +=  RxFrameData[i+3];										/* 记录接收的数据大小 */
										memcpy(receiveDataPtr,&RxFrameData[i+4],RxFrameData[i+3]);	/* 保存固件文件 */
										receiveDataPtr += RxFrameData[i+3];
										break;
								}
								case CMD_SEND_BIN_FILE_END:		/* 固件文件接收完毕 */
								{
										receiveDataPtr = NULL;
										if(binFilePackReceivedSize == binFileSize)	/* 判断实际接收是否等于预先发送的文件大小 */
										{
											sprintf(logBuffer,"\xe5\x9b\xba\xe4\xbb\xb6\xe6\x8e\xa5\xe6\x94\xb6\xe5\xae\x8c\xe6\x88\x90\x2c\xe5\xa4\xa7\xe5\xb0\x8f\xe4\xb8\xba\x3a%d\x62\x79\x74\x65",binFilePackReceivedSize);/* 固件接收完成,大小为： byte */
											sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
											
											binFileReceiveOK = 1; /* 固件接收完成标志置位 */
											
										}
										else
										{
											sprintf(logBuffer,"\xe5\x9b\xba\xe4\xbb\xb6\xe6\x8e\xa5\xe6\x94\xb6\xe6\x95\xb0\xe6\x8d\xae\xe4\xb8\x8d\xe5\xae\x8c\xe6\x95\xb4\xef\xbc\x8c\xe5\xae\x9e\xe9\x99\x85\xe6\x8e\xa5\xe6\x94\xb6\x3a%d\x62\x79\x74\x65",binFilePackReceivedSize);/* 固件接收数据不完整，实际接收: byte */
											sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
										}
											
										break;
								}
								case CMD_SHOW_DEVICE_INFO:
								{
										sendDeviceInfoFlag = 1;
										break;
								}
								default:
										break;
						}

						i += OneFrameDataLen-1;										/*偏移*/
					}
				}
				else
				{
					WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
					break;
				}
			}
		}
	}
}

static uint8_t sendLogMessage(uint8_t frameHead, uint8_t frameAddress, uint8_t frameID, char* data)
{
    uint8_t frameDataLen = 0;
		uint32_t cnt = 0;
    char frameData[300];
		memset(frameData,0,sizeof(frameData));
    frameDataLen = strlen(data);
    frameData[cnt++] = frameHead;                               /* 帧头 */
    frameData[cnt++] = frameAddress;                            /* 帧地址 */
    frameData[cnt++] = frameID;                                 /* 帧ID */
    frameData[cnt++] = frameDataLen;                            /* 帧数据长度 */

		memcpy(&frameData[4],data,frameDataLen);
	
    uint8_t sumcheck = 0;
    uint8_t addcheck = 0;
    for(uint8_t i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
		
		while(USBD_OK != CDC_Transmit_HS((uint8_t *)frameData,frameDataLen + 4 + 2)){};
    return frameDataLen + 4 + 2;
}

static void JumpToApp(void)
{
	/* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
	if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
	{
		/* Jump to user application */
		JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
		JumpToApplication = (pFunction) JumpAddress;
		/* Initialize user application's Stack Pointer */
		__set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
		JumpToApplication();
	}

	/* 跳转成功的话，不会执行到这里，用户可以在这里添加代码 */
	while (1)
	{

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
	
	/* 发送当前运行在IAP程序的状态 */
	vcpSendData(0xAA,MCU,CMD_NOW_IAP_STATE,0,0);
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
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM17 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM17) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
