/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ringBuffer.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void vcpDataAnalysis(_RingBuffer *ringbuffer);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
/* ����ȫ���жϵĺ� */
#define ENABLE_INT()	__set_PRIMASK(0)	/* ʹ��ȫ���ж� */
#define DISABLE_INT()	__set_PRIMASK(1)	/* ��ֹȫ���ж� */

#define BIN_FILE_MAX_SIZE (512*1024)									/* ���̼���С */
#define APPLICATION_ADDRESS   (uint32_t)0x08020000		/* APP���еĵ�ַ */	
typedef struct 
{
		char wifiName[50];											/* WIFI���� */
		char wifiPssword[50];										/* WIFI���� */
    uint16_t picSize;                  			/* ͼƬ�ĸ�ʽ */
    unsigned int picWidth;                  /* ͼƬ�ĳ� */
    unsigned int picHeight;                 /* ͼƬ�Ŀ� */
    unsigned int scheduledDeletion;         /* ��ʱɾ��ʱ�� */
    char deviceID[50];                      /* �豸ID */
    int jpegQuality;                        /* JPEGͼ������ */
    char serverIP[20];                  		/* ip��ַ */
    unsigned int serverPort;           			/* �˿�  */
		char isSetRecordTime;										/* ��¼��ʱ���Ƿ��Ѿ����ù��� */
    char recordTime[50][6];                 /* ��ʱ�� */
    int recordTimeIndex;                    /* ��¼�̶���ʱ��ʱ���� */
    unsigned int ledFlashBrightness;        /* ��������� */
    unsigned int takePictureDelayTime;     	/* ���յ���ʱʱ�� */
		uint8_t sysState;												/* ϵͳ״̬ */
		uint8_t cameraState;										/* ���״̬ */
		uint8_t air724State;										/* AIR724�Ĺ���״̬ */
		uint8_t iapState;												/* IAP״̬ */
		uint8_t automaticFocus;									/* �Զ��Խ� */
		uint8_t pictureDirection;								/* �Զ��Խ� */
}_deviceInfo;
extern _deviceInfo deviceAttributeInfo;

enum frameAddress
{
	PC = 0xA5,
	MCU = 0xA6
};

enum frameCmd
{
	CDM_WIFI_NAME = 0,/* ֡Я��������ΪWIFI������ */
	CMD_WIFI_PASSWORD,
	CMD_SERVER_IP,
	CMD_SERVER_PORT,
	CMD_PICTURE_SIZE,
	CMD_PICTURE_QUALITY,
	CMD_SET_PARA_END,
	CMD_LOG_MESSAGE,
	CMD_DEVICE_ID,
	CMD_SHOW_DEVICE_INFO,
	CMD_RESET_DEVICE,
	CMD_SET_RECORD_TIME,
	CMD_SET_DELAY_TIME,
	CMD_SET_LIED_BRIGHTNESS,
	CMD_SET_TAKE_PICTURE_DELAY_TIME,
	CMD_SET_TEST_MODE,									/* �����豸��ǰ�����ڲ���ģʽ�� */
	CMD_SET_WORK_MODE,									/* �����豸��ǰ�����ڹ���ģʽ�� */
	CMD_TAKE_PICTURE_TEST_MODE,					/* ����ģʽ�µ����� */
	CMD_OPEN_MOTO_TEST_MODE,						/* ����ģʽ�µĴ򿪵�� */
	CMD_JPEG_DATA_PACK_SIZE,						/* �豸����λ������һ��JPEG��ʽͼƬ�Ĵ�С */
	CMD_JPEG_DATA_PACK,									/* �豸����λ������JPEG���ݰ� */
	CMD_JPEG_DATA_SEND_END,							/* �豸֪ͨ��λ��JPEG���ݷ��ͽ��� */
	CMD_SEND_BIN_FILE_SIZE,             /* ���͵Ĺ̼��ļ���С */
	CMD_SEND_BIN_FILE_PACK,             /* �̼����ݰ� */
	CMD_SEND_BIN_FILE_END,              /* �̼����ݷ������ */
	CMD_RUN_IAP,                        /* ����IAP���� */
	CMD_NOW_IAP_STATE,									/* ��ǰ����IAP���� */
	CMD_NOW_APP_STATE,									/* ��ǰ����APP���� */
	CMD_SET_AUTO_FOCUS,                 /* �Զ��Խ� */
	CMD_SET_CAMERA_PICTURE_DIRECTION		/* ����ͼƬ���� */
};

enum iapState
{
	IAP_RUN_STATE = 0,
	APP_RUN_STATE
};
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
