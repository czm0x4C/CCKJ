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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define OV_RESET_Pin GPIO_PIN_10
#define OV_RESET_GPIO_Port GPIOI
#define OV_PWDN_Pin GPIO_PIN_11
#define OV_PWDN_GPIO_Port GPIOI
#define IN1_Pin GPIO_PIN_7
#define IN1_GPIO_Port GPIOC
#define MOTO_CTR_Pin GPIO_PIN_1
#define MOTO_CTR_GPIO_Port GPIOI
#define AIR724_RESET_Pin GPIO_PIN_6
#define AIR724_RESET_GPIO_Port GPIOI
#define POWERKEY_Pin GPIO_PIN_7
#define POWERKEY_GPIO_Port GPIOI

/* USER CODE BEGIN Private defines */
#define EXT_SDRAM_ADDR  	(0xC0000000)
#define EXT_SDRAM_SIZE		(32 * 1024 * 1024)

#define RGB_DATA_BUFFER_SIZE (16 * 1024 * 1024)
#define JPG_DATA_BUFFER_SIZE (16 * 1024 * 1024)

extern SDRAM_HandleTypeDef hsdram1;


#define jpeg_buf_size   128*1024		//����JPEG���ݻ���jpeg_buf�Ĵ�С(1*4M�ֽ�)
extern uint32_t dcmi_data_buff[RGB_DATA_BUFFER_SIZE/4]     __attribute__((at(EXT_SDRAM_ADDR)));

extern uint32_t jpgDataBuffer[JPG_DATA_BUFFER_SIZE/4]     __attribute__((at(EXT_SDRAM_ADDR + RGB_DATA_BUFFER_SIZE)));

extern void jpeg_data_process(void);
extern void jpeg_dcmi_rx_callback(void);

extern volatile uint8_t jpeg_data_ok;
extern volatile unsigned int jpeg_data_len;
extern unsigned char *picStart;

extern uint8_t sendRgbPicData;

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
}_deviceInfo;
extern _deviceInfo deviceAttributeInfo;

typedef struct 
{
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
}MY_TIM;
extern MY_TIM time;
enum frameAddress
{
	PC = 0xA5,
	MCU = 0xA6
};
enum framesize
{
	FRAMESIZE_96X96,    // 96x96
	FRAMESIZE_QQVGA,    // 160x120
	FRAMESIZE_QCIF,     // 176x144
	FRAMESIZE_HQVGA,    // 240x176
	FRAMESIZE_240X240,  // 240x240
	FRAMESIZE_QVGA,     // 320x240
	FRAMESIZE_CIF,      // 400x296
	FRAMESIZE_HVGA,     // 480x320
	FRAMESIZE_VGA,      // 640x480
	FRAMESIZE_SVGA,     // 800x600
	FRAMESIZE_XGA,      // 1024x768
	FRAMESIZE_HD,       // 1280x720
	FRAMESIZE_SXGA,     // 1280x1024
	FRAMESIZE_UXGA,     // 1600x1200
	// 3MP Sensors
	FRAMESIZE_FHD,      // 1920x1080
	FRAMESIZE_P_HD,     //  720x1280
	FRAMESIZE_P_3MP,    //  864x1536
	FRAMESIZE_QXGA,     // 2048x1536
	// 5MP Sensors
	FRAMESIZE_QHD,      // 2560x1440
	FRAMESIZE_WQXGA,    // 2560x1600
	FRAMESIZE_P_FHD,    // 1080x1920
	FRAMESIZE_QSXGA,    // 2560x1920
	FRAMESIZE_INVALID
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
	CMD_JPEG_DATA_SEND_END							/* �豸֪ͨ��λ��JPEG���ݷ��ͽ��� */
};

enum
{
	/* ��ɫ */
	DEVICE_LABEL_PC = 0x00,             /* �豸��ǩPC�� */
	DEVICE_LABEL_CAMERA,                /* �豸��ǩCAMERA�� */
	DEVICE_LABEL_SERVER,                /* �豸��ǩSERVER�� */
	/* target */
	PC_TO_SERVER,                       /* PC��SERVER */
	PC_TO_CAMER,                        /* PC��CAMERA */
	CAMER_TO_SERVER,                    /* CAMERA��SERVER */
	CAMER_TO_PC,                        /* CAMERA��PC */
	SERVER_TO_CLIENT,                   /* SERVER��CLIENT */
	/* cmd */
	HERAT_BEAT_PACK,                    /* ������ */
	SET_PC_DEVICE_FLAG,                 /* ����PC��ʶ */
	SET_CAMERA_DEVICE_FLAG,             /* ����CAMERA��ʶ */
	SET_CAMERA_DEVICE_ID,               /* ���������ID */
	GET_ONLINE_CAMERA_DEVICE_ID,        /* �����������ӵ�camera�豸ID */
	TAKE_PICTURE,                       /* �������� */
	TAKE_PICTURE_END,                   /* ���ս��� */
	TAKE_PICTURE_ERROR,                 /* ���մ��� */
	CAMERA_ID,                          /* ���ID */
	SEND_CAMERA_ID_END,                 /* �������ID���� */
	CLIENT_BIND_CAMERA,                 /* �ͻ��˰���� */
	CLIENT_BIND_CAMERA_OK,              /* �ͻ��˰�����ɹ� */
	CLIENT_DISBIND_CAMERA,              /* �ͻ��˽������ͷ */
	CLIENT_DISBIND_CAMERA_OK,           /* �ͻ��˽������ͷ�ɹ� */
	CLIENT_EXIST_PICTURE_FILE_NAME,     /* �ͻ��˴��ڵ�ͼƬ�ļ����� */
	CLIENT_EXIST_PICTURE_FILE_NAME_END, /* �ͻ��˴��ڵ�ͼƬ�ļ����Ʒ��ͽ��� */
	DOWNLOAD_PICTURE,                   /* �ͻ�����������ͼƬ���� */
	DOWNLOAD_PICTURE_END,               /* �ͻ�����������ͼƬ���ݽ��� */
	DOWNLOAD_PICTURE_EMPTY,             /* �ͻ�����������ͼƬ������Ϊ�� */
	PICTURE_TO_PC_NAME,                 /* ��ͼƬ���ַ��͵�PC�� */
	PICTURE_TO_PC_DATA,                 /* ��ͼ�����ݷ���PC�� */
	SET_RECORD_TIME,                    /* ���ö�ʱʱ�� */
	SET_RECORD_TIME_END,                /* ���ö�ʱ���� */
	SET_RECORD_TIME_SUCCESS,            /* ���ö�ʱʱ��ɹ����� */
	SET_SCHEDULED_TIME,                 /* ���ü����ʱʱ�� */
	SET_SCHEDULED_TIME_CMD_SUCCESS,     /* ���ü����ʱʱ��ɹ����� */
	SAVE_PC_PICTURE,                    /* ������PC�˷�����ͼƬ */
	SET_TEMP_DEVICE_ID,                 /* δ��ʱ����ʱ���ð�ID */
	CLEAR_SERVER_CACHE,                 /* ����������ϵ�ͼƬ���� */
	CLEAR_SERVER_CACHE_DONE,            /* ���������� */
	OPEN_MOTO_CMD,                      /* ��ˮ�õ����� */
	OPEN_MOTO_SUCCESS_CMD,              /* �򿪳ɹ����� */
	OPEN_MOTO_FAIL_CMD,                 /* ��ʧ�ܷ��� */
	PICTURE_DATA_PACK,                  /* ͼ�����ݰ� */
	PICTURE_DATA_PACK_INDEX,						/* ͼ�����ݰ���� */
	TAKE_RGB_PICTURE_END,								/* ����RGBͼƬ���� */
	SET_LIED_BRIGHTNESS_CMD,
	SET_TAKE_PICTURE_DELAY_TIME_CMD
};


enum sysState
{
	SYS_OK_STATE = 0,										/* �豸ϵͳ����������ͬʱҲ�ǿ���״̬ */
	SYS_TEST_MODE_SATE,									/* �豸�����ڲ���״̬ */
};

enum camerState
{
	CAMERA_OK_STATE = 0,
	CAMERA_ERROR_STATE
};

enum air724State
{
	AIR724_OK_STATE = 0,
	AIR724_ERROR_STATE
};

extern char anlysis;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
