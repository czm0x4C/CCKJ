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
/* 开关全局中断的宏 */
#define ENABLE_INT()	__set_PRIMASK(0)	/* 使能全局中断 */
#define DISABLE_INT()	__set_PRIMASK(1)	/* 禁止全局中断 */

#define BIN_FILE_MAX_SIZE (512*1024)									/* 最大固件大小 */
#define APPLICATION_ADDRESS   (uint32_t)0x08020000		/* APP运行的地址 */	
typedef struct 
{
		char wifiName[50];											/* WIFI名字 */
		char wifiPssword[50];										/* WIFI密码 */
    uint16_t picSize;                  			/* 图片的格式 */
    unsigned int picWidth;                  /* 图片的长 */
    unsigned int picHeight;                 /* 图片的宽 */
    unsigned int scheduledDeletion;         /* 定时删除时间 */
    char deviceID[50];                      /* 设备ID */
    int jpegQuality;                        /* JPEG图像质量 */
    char serverIP[20];                  		/* ip地址 */
    unsigned int serverPort;           			/* 端口  */
		char isSetRecordTime;										/* 记录定时组是否已经设置过了 */
    char recordTime[50][6];                 /* 定时组 */
    int recordTimeIndex;                    /* 记录固定定时的时间组 */
    unsigned int ledFlashBrightness;        /* 闪光灯亮度 */
    unsigned int takePictureDelayTime;     	/* 拍照的延时时间 */
		uint8_t sysState;												/* 系统状态 */
		uint8_t cameraState;										/* 相机状态 */
		uint8_t air724State;										/* AIR724的工作状态 */
		uint8_t iapState;												/* IAP状态 */
		uint8_t automaticFocus;									/* 自动对焦 */
		uint8_t pictureDirection;								/* 自动对焦 */
}_deviceInfo;
extern _deviceInfo deviceAttributeInfo;

enum frameAddress
{
	PC = 0xA5,
	MCU = 0xA6
};

enum frameCmd
{
	CDM_WIFI_NAME = 0,/* 帧携带的数据为WIFI的名称 */
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
	CMD_SET_TEST_MODE,									/* 设置设备当前工作在测试模式下 */
	CMD_SET_WORK_MODE,									/* 设置设备当前工作在工作模式下 */
	CMD_TAKE_PICTURE_TEST_MODE,					/* 测试模式下的拍照 */
	CMD_OPEN_MOTO_TEST_MODE,						/* 测试模式下的打开电机 */
	CMD_JPEG_DATA_PACK_SIZE,						/* 设备向上位机发送一张JPEG格式图片的大小 */
	CMD_JPEG_DATA_PACK,									/* 设备向上位机发送JPEG数据包 */
	CMD_JPEG_DATA_SEND_END,							/* 设备通知上位机JPEG数据发送结束 */
	CMD_SEND_BIN_FILE_SIZE,             /* 发送的固件文件大小 */
	CMD_SEND_BIN_FILE_PACK,             /* 固件数据包 */
	CMD_SEND_BIN_FILE_END,              /* 固件数据发送完成 */
	CMD_RUN_IAP,                        /* 运行IAP程序 */
	CMD_NOW_IAP_STATE,									/* 当前运行IAP程序 */
	CMD_NOW_APP_STATE,									/* 当前运行APP程序 */
	CMD_SET_AUTO_FOCUS,                 /* 自动对焦 */
	CMD_SET_CAMERA_PICTURE_DIRECTION		/* 设置图片方向 */
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
