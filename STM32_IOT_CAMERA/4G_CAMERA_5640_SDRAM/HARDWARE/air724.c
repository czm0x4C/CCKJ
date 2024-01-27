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
/* air724��ʼ������ */
uint8_t air724Init(void)
{
	/* Ӳ������ */
	HAL_GPIO_WritePin(POWERKEY_GPIO_Port, POWERKEY_Pin, GPIO_PIN_SET);
	vTaskDelay(pdMS_TO_TICKS(2000));
	HAL_GPIO_WritePin(POWERKEY_GPIO_Port, POWERKEY_Pin, GPIO_PIN_RESET);/* ʹ��AIR724���� */
	vTaskDelay(pdMS_TO_TICKS(5000));
	char errorAckBuffer[50];
	char cmdBuffer[50];
	char logBuffer[100];
	uint8_t reTryCnt = 0;																				/* ���Դ��� */
																															/* ��Ƭ����USART1 Ĭ�Ϲ�����UARTģʽ */
	air724Struct.state = 1;																			/* ��ǰΪָ�����״̬ */
	air724Struct.uartMode = UART_HWCONTROL_NONE;								/* ��������ͨģʽ */
	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x41\x49\x52\x37\x32\x34\xe5\x90\xaf\xe5\x8a\xa8");/* AIR724���� */
	
	/* ATָ��ѵ�� */
	while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
	{
		if(++reTryCnt == 3)break;																		/* ��������ʧ�� */
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
		
		air724Struct.uartMode = UART_HWCONTROL_RTS_CTS;							/* ϣ��������Ӳ������ģʽ������Ӳ������ģʽ */
		reTryCnt = 0;
		while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
		{
			if(++reTryCnt == 5)return AIR724_INIT_FAIL;;							/* 5������ʧ�� */
		}
	}
	else
	{
		char *ackP = air724SendCmdAndReturnAck((const char*)"AT+IFC?\r\n",(const char*)"+IFC:",5000);
		if(NULL == ackP)return AIR724_INIT_FAIL;
		if(ackP[6] != '2' || ackP[8] != '2') 
		{
			CDC_Transmit_HS((uint8_t *)"��Ҫ����\r\n",10);
			if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+IFC=2,2;&W\r\n",(const char *)"OK",5000))/* ����ģ��UARTΪӲ������ģʽ�����Ҷϵ籣������ */
			{
				CDC_Transmit_HS((uint8_t *)"����ʧ��\r\n",10);
				return AIR724_INIT_FAIL;
			}
			__HAL_UART_DISABLE(&huart1);
			__HAL_UART_HWCONTROL_CTS_ENABLE(&huart1);
			__HAL_UART_HWCONTROL_RTS_ENABLE(&huart1);
			__HAL_UART_ENABLE(&huart1);
			air724Struct.uartMode = UART_HWCONTROL_RTS_CTS;
			if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+RESET\r\n",(const char *)"OK",5000))
			{
				CDC_Transmit_HS((uint8_t *)"����ʧ��\r\n",10);
				return AIR724_INIT_FAIL;
			}
			
			reTryCnt = 0;
			while(AIR724_ACK_OK != air724SendCmd((const char *)"AT\r\n",(const char *)"OK",5000))
			{
				if(++reTryCnt == 5)return AIR724_INIT_FAIL;;							/* 5������ʧ�� */
			}
			/* ִ�е����ģ���Ѿ�����˴�uart��uart��Ӳ���������� */
		}
	}
	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe6\xa8\xa1\xe5\x9d\x97\xe5\xb7\xa5\xe4\xbd\x9c\xe6\xad\xa3\xe5\xb8\xb8");/* ģ�鹤������ */
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+IPR=921600\r\n",(const char *)"OK",5000))		/* ���ò����� */
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
	
	/* ���͵�ǰ���õ�ϵͳʱ�� */
	sprintf(logBuffer,"\xe7\xb3\xbb\xe7\xbb\x9f\xe6\x97\xb6\xe9\x97\xb4\xe4\xb8\xba\x3a %02d\x3a%02d\x3a%02d",time.hour,time.minute,time.second);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CPIN?\r\n",(const char *)"READY",5000))		/* ��鿨�Ƿ���� */
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\x8d\xa1\xe6\x9c\xaa\xe6\x8f\x92\xe5\x85\xa5");/* ��δ���� */
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CSQ\r\n",(const char*)"+CSQ:",5000);	/* ����ź�ǿ�� */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	uint8_t rssi = (ackP[6] - 0x30) * 10 + (ackP[7] - 0x30);																															
	if(0 == rssi)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe5\xbd\x93\xe5\x89\x8d\xe8\xae\xbe\xe5\xa4\x87\xe4\xbf\xa1\xe5\x8f\xb7\xe5\xbc\xba\xe5\xba\xa6\xe5\xbc\x82\xe5\xb8\xb8");/* ��ǰ�豸�ź�ǿ���쳣  */
		return AIR724_INIT_FAIL;
	}		
	/* ���͵�ǰ�ź�ǿ�� */
	sprintf(logBuffer,"\xe5\xbd\x93\xe5\x89\x8d\xe8\xae\xbe\xe5\xa4\x87\xe4\xbf\xa1\xe5\x8f\xb7\xe5\xbc\xba\xe5\xba\xa6\x3a%d",rssi);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);/* ��ǰ�豸�ź�ǿ��:  */
	
	/* ��ʼ����ģ�����ӵ�TCP������ */
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CREG?\r\n",(const char *)"OK",5000))			/* ��ѯGPRSע��״̬ */
	{
		sprintf(errorAckBuffer,"%s","AT+CREG ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CGATT?\r\n",(const char*)"+CGATT:",5000);		/* ���GPRS���� */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	if(ackP[8] != '1')
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x47\x50\x52\x53\xe9\x99\x84\xe7\x9d\x80\xe5\xa4\xb1\xe8\xb4\xa5");/* GPRS����ʧ�� */
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMUX=0\r\n",(const char *)"OK",5000))		/* ����Ϊ������ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMUX=0 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMODE=1\r\n",(const char *)"OK",5000))	/* ����TCPIPӦ��Ϊ͸������ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMODE=1 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPQSEND=1\r\n",(const char *)"OK",5000))	/* ����Ϊ�췢ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPQSEND=1 ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CSTT\r\n",(const char *)"OK",5000))				/* ������������APN */
	{
		sprintf(errorAckBuffer,"%s","AT+CSTT ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIICR\r\n",(const char *)"OK",5000))			/* �����ƶ���������ȡIP��ַ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIICR ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CIFSR\r\n",(const char*)"\r\n",5000);				/* ��ȡIP */
	if(NULL == ackP)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\x49\x50\xe8\x8e\xb7\xe5\x8f\x96\xe5\xa4\xb1\xe8\xb4\xa5");/* IP��ȡʧ�� */
		return AIR724_INIT_FAIL;
	}
	ackP = ackP+2;
	sprintf(logBuffer,"GET IP: %s",ackP);
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,logBuffer);
	
	reTryCnt = 0;
	while(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPSTATUS\r\n",(const char *)"IP STATUS",5000) && reTryCnt<5)	/* �����ƶ���������ȡIP��ַ */
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
	while(AIR724_ACK_OK != air724SendCmd((const char *)cmdBuffer,(const char *)"CONNECT",5000)&& reTryCnt<5)		/* ���ӷ����� */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPSTART ERROR\r\n");
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,errorAckBuffer);
		HAL_Delay(100);
		reTryCnt++;
	}
	if(5 == reTryCnt)
	{
		sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe8\xbf\x9e\xe6\x8e\xa5\xe6\x9c\x8d\xe5\x8a\xa1\xe5\x99\xa8\xe5\xa4\xb1\xe8\xb4\xa5");/* ���ӷ�����ʧ�� */
		return AIR724_INIT_FAIL;
	}
	reTryCnt = 0;
	
	anlysis = 1;																																								/* ���Կ�ʼ����TCP���� */	
	sendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,"\xe6\xa8\xa1\xe5\x9d\x97\xe5\x90\xaf\xe5\x8a\xa8\xe5\xae\x8c\xe6\x88\x90");/* ���ӷ�����ʧ�� */
	vTaskDelay(pdMS_TO_TICKS(1000));
	packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_FLAG,0,0);		/* ���������������Ϊcamera��ǩ */
	/* �������ID */
	packTcpDataFrameAndSend(DEVICE_LABEL_CAMERA,CAMER_TO_SERVER,SET_CAMERA_DEVICE_ID,strlen(deviceAttributeInfo.deviceID),(uint8_t *)deviceAttributeInfo.deviceID);
	return AIR724_INIT_OK;
}
/* ����AT����,��ȷ�ϵ�ACK�����ж� */
static uint8_t air724SendCmd(const char *cmd,const char *ack,uint32_t waitTime)
{
	memset(air724Struct.receiveDataBuffer,0,sizeof(air724Struct.receiveDataBuffer));		/* ��ջ����� */
	HAL_UART_Transmit(&huart1,(const uint8_t *)cmd,strlen(cmd),5000);										/* ����ָ�� */
	air724Struct.bufferLen = 0;																													/* �������� */	
	
	if(waitTime<100)
	{
		vTaskDelay(pdMS_TO_TICKS(waitTime));
		if(strstr((const char *)air724Struct.receiveDataBuffer,ack))return AIR724_ACK_OK;
		else return AIR724_ACK_FAIL;
	}
	else
	{
		uint8_t cntTime = waitTime/100;																										/* ����ȴ���ʱ���Ǽ�����100ms */
		while(cntTime--)
		{
			vTaskDelay(pdMS_TO_TICKS(100));
			if(0 != air724Struct.bufferLen)
			{
				if(strstr((const char *)air724Struct.receiveDataBuffer,ack))return AIR724_ACK_OK;/* ���յ���ȷ�Ļ�Ӧ */
			}
		}
		return AIR724_ACK_FAIL;																														/* ��ʱû�н��յ���ȷ�Ļ�Ӧ */
	}
}
/* ����ATָ��,ͬʱ������Ҫ��ACK */
static char* air724SendCmdAndReturnAck(const char *cmd,const char *ack,uint32_t waitTime)
{
	memset(air724Struct.receiveDataBuffer,0,sizeof(air724Struct.receiveDataBuffer));		/* ��ջ����� */
	HAL_UART_Transmit(&huart1,(const uint8_t *)cmd,strlen(cmd),5000);										/* ����ָ�� */
	air724Struct.bufferLen = 0;																													/* �������� */	
	
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
		uint8_t cntTime = waitTime/100;																										/* ����ȴ���ʱ���Ǽ�����100ms */
		while(cntTime--)
		{
			vTaskDelay(pdMS_TO_TICKS(100));
			if(0 != air724Struct.bufferLen)
			{
				if(strstr((const char *)air724Struct.receiveDataBuffer,ack))
					return strstr((const char *)air724Struct.receiveDataBuffer,ack);						/* ���յ���ȷ�Ļ�Ӧ */
			}
		}
		return NULL;																																			/* ��ʱû�н��յ���ȷ�Ļ�Ӧ */
	}
}
/* �����͵�TCP���ݷ�֡��ͬʱ���������ͳ�ȥ */
static void packTcpDataFrameAndSend(uint8_t charactar,uint8_t target, uint8_t cmd, uint32_t dataLen, uint8_t *data)
{
	/* ��ΪTCP���ݰ�����������ͬʱ����Ϊ��Ƭ�����ڴ��᧿�Σ������ʹ�ò�ͬ�ڴ����ʵ�ַ��� */
	/* 0xAA byte1 byte2 byte3  byte4 byte5 byte6 byte7   byteN   0xBB */
	/* ֡ͷ  ��ɫ   Ŀ��   ����       ���ȣ����ֽڣ�          �����غ�   ֡β */
	uint8_t tempData[20];
	uint32_t cnt = 0;
	tempData[cnt++] = 0xAA;                 /* ֡ͷ */
	tempData[cnt++] = charactar;      			/* ��ɫ */
	tempData[cnt++] = target;               /* Ŀ�� */
	tempData[cnt++] = cmd;                  /* ���� */
	tempData[cnt++] = dataLen;              /* ���� */
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
    frameData[cnt++] = frameHead;                               /* ֡ͷ */
    frameData[cnt++] = frameAddress;                            /* ֡��ַ */
    frameData[cnt++] = frameID;                                 /* ֡ID */
    frameData[cnt++] = frameDataLen;                            /* ֡���ݳ��� */

		memcpy(&frameData[4],data,frameDataLen);
	
    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //��֡ͷ��ʼ����ÿһ�ֽڽ�����ͣ�ֱ��DATA������
        addcheck += sumcheck; //ÿһ�ֽڵ���Ͳ���������һ��sumcheck���ۼ�
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
		
		CDC_Transmit_HS((uint8_t *)frameData,frameDataLen + 4 + 2);
    return frameDataLen + 4 + 2;
}

