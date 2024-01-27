#include "air724.h"
//#include "stm32f4xx.h"
//#include "stm32f4xx_hal.h"
#include "stm32h7xx_hal_uart.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "cmsis_armcc.h"
#include "usbd_cdc_if.h"

AIR724_TypeDef air724Struct;

static uint8_t air724SendCmd(const char *cmd,const char *ack,uint32_t waitTime);	
static char* air724SendCmdAndReturnAck(const char *cmd,const char *ack,uint32_t waitTime);
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data);
static uint8_t sendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data);
/* air724初始化配置 */
uint8_t air724Init(void)
{
	/* 硬件重启 */
	HAL_GPIO_WritePin(POWERKEY_GPIO_Port, POWERKEY_Pin, GPIO_PIN_SET);
	vTaskDelay(pdMS_TO_TICKS(2000));
	HAL_GPIO_WritePin(POWERKEY_GPIO_Port, POWERKEY_Pin, GPIO_PIN_RESET);/* 使能AIR724开机 */
	vTaskDelay(pdMS_TO_TICKS(5000));
	char errorAckBuffer[50];
	char cmdBuffer[50];
	char logBuffer[100];
	uint8_t reTryCnt = 0;																				/* 重试次数 */
																															/* 单片机的USART1 默认工作在UART模式 */
	air724Struct.state = 1;																			/* 当前为指令配对状态 */
	air724Struct.uartMode = UART_HWCONTROL_NONE;								/* 工作在普通模式 */
	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x41\x49\x52\x37\x32\x34\xe5\x90\xaf\xe5\x8a\xa8");/* AIR724启动 */
	
	/* AT指令训练 */
	while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
	{
		if(++reTryCnt == 3)break;																		/* 三次重试失败 */
	}
	
	if(3 == reTryCnt)		
	{
		__HAL_UART_DISABLE(&huart1);
		HAL_UART_DeInit(&huart1);
		huart1.Instance = USART1;
		huart1.Init.BaudRate = 115200;
		huart1.Init.WordLength = UART_WORDLENGTH_8B;
		huart1.Init.StopBits = UART_STOPBITS_1;
		huart1.Init.Parity = UART_PARITY_NONE;
		huart1.Init.Mode = UART_MODE_TX_RX;
		huart1.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
		huart1.Init.OverSampling = UART_OVERSAMPLING_16;
		if (HAL_UART_Init(&huart1) != HAL_OK)
		{
			Error_Handler();
		}
		HAL_UART_Receive_IT(&huart1,&uartTempData,1);
		__HAL_UART_ENABLE(&huart1);
		
		air724Struct.uartMode = UART_HWCONTROL_RTS_CTS;							/* 希望工作在硬件流控模式，测试硬件流控模式 */
		reTryCnt = 0;
		while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
		{
			if(++reTryCnt == 5)return AIR724_INIT_FAIL;;							/* 5次重试失败 */
		}
	}
	else
	{
		char *ackP = air724SendCmdAndReturnAck((const char*)"AT+IFC?\r\n",(const char*)"+IFC:",5000);
		if(NULL == ackP)return AIR724_INIT_FAIL;
		if(ackP[6] != '2' || ackP[8] != '2') 
		{
			CDC_Transmit_HS((uint8_t *)"需要设置\r\n",10);
			if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+IFC=2,2;&W\r\n",(const char *)"OK",5000))/* 设置模块UART为硬件流控模式，并且断电保存设置 */
			{
				CDC_Transmit_HS((uint8_t *)"设置失败\r\n",10);
				return AIR724_INIT_FAIL;
			}
			__HAL_UART_DISABLE(&huart1);
			__HAL_UART_HWCONTROL_CTS_ENABLE(&huart1);
			__HAL_UART_HWCONTROL_RTS_ENABLE(&huart1);
			__HAL_UART_ENABLE(&huart1);
			air724Struct.uartMode = UART_HWCONTROL_RTS_CTS;
			if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+RESET\r\n",(const char *)"OK",5000))
			{
				CDC_Transmit_HS((uint8_t *)"重启失败\r\n",10);
				return AIR724_INIT_FAIL;
			}
			
			reTryCnt = 0;
			while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
			{
				if(++reTryCnt == 5)return AIR724_INIT_FAIL;;							/* 5次重试失败 */
			}
			/* 执行到这里，模块已经完成了从uart到uart的硬件流控设置 */
		}
	}
	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe6\xa8\xa1\xe5\x9d\x97\xe5\xb7\xa5\xe4\xbd\x9c\xe6\xad\xa3\xe5\xb8\xb8");/* 模块工作正常 */
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+IPR=921600\r\n",(const char *)"OK",5000))		/* 设置波特率 */
	{
		sprintf(errorAckBuffer,"%s","AT+IPR=921600 ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	__HAL_UART_DISABLE(&huart1);
	__HAL_UART_HWCONTROL_CTS_DISABLE(&huart1);
	__HAL_UART_HWCONTROL_RTS_DISABLE(&huart1);
	__HAL_UART_ENABLE(&huart1);
	
	HAL_UART_DeInit(&huart1);
	huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
	HAL_UART_Receive_IT(&huart1,&uartTempData,1);
	
	char *ackP = air724SendCmdAndReturnAck((const char*)"AT+CCLK?\r\n",(const char*)"+CCLK:",5000);
	if(NULL == ackP)return AIR724_INIT_FAIL;
	if(',' == ackP[16])
	{
		time.hour   = (ackP[17] - 0x30)*10 + (ackP[18] - 0x30);
		time.minute = (ackP[20] - 0x30)*10 + (ackP[21] - 0x30);
		time.second = (ackP[23] - 0x30)*10 + (ackP[24] - 0x30);
	}
	
	/* 发送当前设置的系统时间 */
	sprintf(logBuffer,"\xe7\xb3\xbb\xe7\xbb\x9f\xe6\x97\xb6\xe9\x97\xb4\xe4\xb8\xba\x3a %02d\x3a%02d\x3a%02d",time.hour,time.minute,time.second);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CPIN?\r\n",(const char *)"READY",5000))		/* 检查卡是否插入 */
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x8d\xa1\xe6\x9c\xaa\xe6\x8f\x92\xe5\x85\xa5");/* 卡未插入 */
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CSQ\r\n",(const char*)"+CSQ:",5000);	/* 检查信号强度 */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	uint8_t rssi = (ackP[6] - 0x30) * 10 + (ackP[7] - 0x30);																															
	if(0 == rssi)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\xbd\x93\xe5\x89\x8d\xe8\xae\xbe\xe5\xa4\x87\xe4\xbf\xa1\xe5\x8f\xb7\xe5\xbc\xba\xe5\xba\xa6\xe5\xbc\x82\xe5\xb8\xb8");/* 当前设备信号强度异常  */
		return AIR724_INIT_FAIL;
	}		
	/* 发送当前信号强度 */
	sprintf(logBuffer,"\xe5\xbd\x93\xe5\x89\x8d\xe8\xae\xbe\xe5\xa4\x87\xe4\xbf\xa1\xe5\x8f\xb7\xe5\xbc\xba\xe5\xba\xa6\x3a%d",rssi);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);/* 当前设备信号强度:  */
	
	/* 开始设置模块连接到TCP服务器 */
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CREG?\r\n",(const char *)"OK",5000))			/* 查询GPRS注册状态 */
	{
		sprintf(errorAckBuffer,"%s","AT+CREG ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CGATT?\r\n",(const char*)"+CGATT:",5000);		/* 检查GPRS附着 */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	if(ackP[8] != '1')
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x47\x50\x52\x53\xe9\x99\x84\xe7\x9d\x80\xe5\xa4\xb1\xe8\xb4\xa5");/* GPRS附着失败 */
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMUX=0\r\n",(const char *)"OK",5000))		/* 设置为单链接模式 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMUX=0 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMODE=1\r\n",(const char *)"OK",5000))	/* 设置TCPIP应用为透明传输模式 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMODE=1 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPQSEND=1\r\n",(const char *)"OK",5000))	/* 设置为快发模式 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPQSEND=1 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CSTT\r\n",(const char *)"OK",5000))				/* 启动任务，设置APN */
	{
		sprintf(errorAckBuffer,"%s","AT+CSTT ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIICR\r\n",(const char *)"OK",5000))			/* 激活移动场景，获取IP地址 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIICR ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CIFSR\r\n",(const char*)"\r\n",5000);				/* 获取IP */
	if(NULL == ackP)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x49\x50\xe8\x8e\xb7\xe5\x8f\x96\xe5\xa4\xb1\xe8\xb4\xa5");/* IP获取失败 */
		return AIR724_INIT_FAIL;
	}
	ackP = ackP+2;
	sprintf(logBuffer,"GET IP: %s",ackP);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	reTryCnt = 0;
	while(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPSTATUS\r\n",(const char *)"IP STATUS",5000) && reTryCnt<5)	/* 激活移动场景，获取IP地址 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPSTATUS ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		HAL_Delay(100);
		reTryCnt++;
	}
	if(5 == reTryCnt)
	{
		return AIR724_INIT_FAIL;
	}
	reTryCnt = 0;
	
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8\xE5\x9C\xB0\xE5\x9D\x80\xE4\xB8\xBA\x3A%s",deviceAttributeInfo.serverIP);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	HAL_Delay(100);
	memset(logBuffer,0,sizeof(logBuffer));
	sprintf(logBuffer,"\xE6\x9C\x8D\xE5\x8A\xA1\xE5\x99\xA8\xE7\xAB\xAF\xE5\x8F\xA3\xE4\xB8\xBA\x3A%04d",deviceAttributeInfo.serverPort);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	sprintf(cmdBuffer,"AT+CIPSTART=\"TCP\",%s,%d\r\n",deviceAttributeInfo.serverIP,deviceAttributeInfo.serverPort );
	while(AIR724_ACK_OK != air724SendCmd((const char *)cmdBuffer,(const char *)"CONNECT",5000)&& reTryCnt<5)		/* 连接服务器 */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPSTART ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		HAL_Delay(100);
		reTryCnt++;
	}
	if(5 == reTryCnt)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe8\xbf\x9e\xe6\x8e\xa5\xe6\x9c\x8d\xe5\x8a\xa1\xe5\x99\xa8\xe5\xa4\xb1\xe8\xb4\xa5");/* 连接服务器失败 */
		return AIR724_INIT_FAIL;
	}
	reTryCnt = 0;
	
	anlysis = 1;																																								/* 可以开始分析TCP数据 */	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe6\xa8\xa1\xe5\x9d\x97\xe5\x90\xaf\xe5\x8a\xa8\xe5\xae\x8c\xe6\x88\x90");/* 连接服务器失败 */
	vTaskDelay(pdMS_TO_TICKS(1000));
	packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_FLAG,0,0);		/* 向服务器设置自身为camera标签 */
	/* 设置相机ID */
	packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_ID,strlen(deviceAttributeInfo.deviceID),(uint8_t *)deviceAttributeInfo.deviceID);
	return AIR724_INIT_OK;
}
/* 发送AT命令,对确认的ACK进行判断 */
static uint8_t air724SendCmd(const char *cmd,const char *ack,uint32_t waitTime)
{
	memset(air724Struct.receiveDataBuffer,0,sizeof(air724Struct.receiveDataBuffer));		/* 清空缓冲区 */
	HAL_UART_Transmit(&huart1,(const uint8_t *)cmd,strlen(cmd),5000);										/* 发送指令 */
	air724Struct.bufferLen = 0;																													/* 计数清零 */	
	
	if(waitTime<100)
	{
		vTaskDelay(pdMS_TO_TICKS(waitTime));
		if(strstr((const char *)air724Struct.receiveDataBuffer,ack))return AIR724_ACK_OK;
		else return AIR724_ACK_FAIL;
	}
	else
	{
		uint8_t cntTime = waitTime/100;																										/* 计算等待的时间是几倍的100ms */
		while(cntTime--)
		{
			vTaskDelay(pdMS_TO_TICKS(100));
			if(0 != air724Struct.bufferLen)
			{
				if(strstr((const char *)air724Struct.receiveDataBuffer,ack))return AIR724_ACK_OK;/* 接收到正确的回应 */
			}
		}
		return AIR724_ACK_FAIL;																														/* 超时没有接收到正确的回应 */
	}
}
/* 发送AT指令,同时返回想要的ACK */
static char* air724SendCmdAndReturnAck(const char *cmd,const char *ack,uint32_t waitTime)
{
	memset(air724Struct.receiveDataBuffer,0,sizeof(air724Struct.receiveDataBuffer));		/* 清空缓冲区 */
	HAL_UART_Transmit(&huart1,(const uint8_t *)cmd,strlen(cmd),5000);										/* 发送指令 */
	air724Struct.bufferLen = 0;																													/* 计数清零 */	
	
	if(waitTime<100)
	{
		vTaskDelay(pdMS_TO_TICKS(waitTime));
		if(0 != air724Struct.bufferLen)
			return strstr((const char *)air724Struct.receiveDataBuffer,ack);
		else 
			return NULL;
	}
	else
	{
		uint8_t cntTime = waitTime/100;																										/* 计算等待的时间是几倍的100ms */
		while(cntTime--)
		{
			vTaskDelay(pdMS_TO_TICKS(100));
			if(0 != air724Struct.bufferLen)
			{
				if(strstr((const char *)air724Struct.receiveDataBuffer,ack))
					return strstr((const char *)air724Struct.receiveDataBuffer,ack);						/* 接收到正确的回应 */
			}
		}
		return NULL;																																			/* 超时没有接收到正确的回应 */
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
		
		CDC_Transmit_HS((uint8_t *)frameData,frameDataLen + 4 + 2);
    return frameDataLen + 4 + 2;
}

