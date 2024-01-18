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
	uint8_t reTryCnt = 0;																				/* ���Դ��� */
																															/* ��Ƭ����USART1 Ĭ�Ϲ�����UARTģʽ */
	air724Struct.state = 1;																			/* ��ǰΪָ�����״̬ */
	air724Struct.uartMode = UART_HWCONTROL_NONE;								/* ��������ͨģʽ */
	
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
	
	CDC_Transmit_HS((uint8_t *)"ģ�鹤������\r\n",14);
	
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
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CPIN?\r\n",(const char *)"READY",5000))		/* ��鿨�Ƿ���� */
	{
		CDC_Transmit_HS((uint8_t *)"��δ����\r\n",10);
		return AIR724_INIT_FAIL;
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CSQ\r\n",(const char*)"+CSQ:",5000);	/* ����ź�ǿ�� */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	uint8_t rssi = (ackP[6] - 0x30) * 10 + (ackP[7] - 0x30);																															
	if(0 == rssi)return AIR724_INIT_FAIL;																													/* �ź�ǿ���쳣 */
	
	/* ��ʼ����ģ�����ӵ�TCP������ */
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CREG?\r\n",(const char *)"OK",5000))			/* ��ѯGPRSע��״̬ */
	{
		sprintf(errorAckBuffer,"%s","AT+CREG? ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CGATT?\r\n",(const char*)"+CGATT:",5000);		/* ���GPRS���� */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	if(ackP[8] != '1')return AIR724_INIT_FAIL;
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMUX=0\r\n",(const char *)"OK",5000))		/* ����Ϊ������ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMUX=0 ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPMODE=1\r\n",(const char *)"OK",5000))	/* ����TCPIPӦ��Ϊ͸������ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPMODE=1 ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPQSEND=1\r\n",(const char *)"OK",5000))	/* ����Ϊ�췢ģʽ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPQSEND=1 ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CSTT\r\n",(const char *)"OK",5000))				/* ������������APN */
	{
		sprintf(errorAckBuffer,"%s","AT+CSTT ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	if(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIICR\r\n",(const char *)"OK",5000))			/* �����ƶ���������ȡIP��ַ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIICR ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	}
	
	ackP = air724SendCmdAndReturnAck((const char*)"AT+CIFSR\r\n",(const char*)"\r\n",5000);				/* ���GPRS���� */
	if(NULL == ackP)return AIR724_INIT_FAIL;
	ackP = ackP+2;
	sprintf(errorAckBuffer,"GET IP: %s",ackP);
	CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
	
	reTryCnt = 0;
	while(AIR724_ACK_OK != air724SendCmd((const char *)"AT+CIPSTATUS\r\n",(const char *)"IP STATUS",5000) && reTryCnt<5)	/* �����ƶ���������ȡIP��ַ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPSTATUS ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
		reTryCnt++;
	}
	
	reTryCnt = 0;
	sprintf(cmdBuffer,"AT+CIPSTART=\"TCP\",%s,%d\r\n",deviceAttributeInfo.serverIP,deviceAttributeInfo.serverPort );
	while(AIR724_ACK_OK != air724SendCmd((const char *)cmdBuffer,(const char *)"CONNECT",5000)&& reTryCnt<5)		/* �����ƶ���������ȡIP��ַ */
	{
		sprintf(errorAckBuffer,"%s","AT+CIPSTART ERROR\r\n");
		CDC_Transmit_HS((uint8_t *)errorAckBuffer,strlen(errorAckBuffer));
		reTryCnt++;
	}
	
	anlysis = 1;																																								/* ���Կ�ʼ����TCP���� */	
	CDC_Transmit_HS((uint8_t *)"ģ���������\r\n",14);
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



