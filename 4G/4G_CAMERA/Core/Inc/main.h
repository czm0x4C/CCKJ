/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

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
#define MOTO_CTR_Pin GPIO_PIN_13
#define MOTO_CTR_GPIO_Port GPIOD
#define IN1_Pin GPIO_PIN_15
#define IN1_GPIO_Port GPIOA
#define RESET_Pin GPIO_PIN_0
#define RESET_GPIO_Port GPIOD
#define PWDN_Pin GPIO_PIN_1
#define PWDN_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */


#define PictureSize (22 * 1024)

extern unsigned char DMA_END_FLAG;
extern unsigned int dcmi_data_buff[PictureSize];
extern unsigned char jpeg_data_ok;
extern unsigned int jpeg_data_len;

typedef struct 
{
    char picFormat[20];                     /* 图片的格式 */
    unsigned int picWidth;                  /* 图片的长 */
    unsigned int picHeight;                 /* 图片的宽 */
    unsigned int scheduledDeletion;         /* 定时删除时间 */
    char deviceID[50];                      /* 设备ID */
    int jpegQuality;                        /* JPEG图像质量 */
    char UDP_serverIP[20];                  /* udp ip地址 */
    unsigned int UDP_serverPort;            /* udp 端口  */
    char recordTime[50][6];                 /* 定时组 */
    int recordTimeIndex;                    /* 记录固定定时的时间组 */
    unsigned int ledFlashBrightness;               /* 闪光灯亮度 */
    unsigned int takePictureDelayTime;     /* 拍照的延时时间 */
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
	CMD_SET_TAKE_PICTURE_DELAY_TIME
};

enum
{
	HERAT_BEAT_PACK = 0x00,
	PICTURE_DATA,
	DOWNLOAD_PICTURE,
	EMPTY,
	PICTURE_TO_CLIENT_NAME,
	PICTURE_TO_CLIENT_DATA,
	PICTURE_TO_CLIENT_END,
	TAKE_PICTURE,
	SET_CAMERA_DEVICE_FLAG,
	SET_CAMERA_DEVICE_ID,
	CAMERA_TAKE_PICTURE,
	CAMERA_TAKE_PICTURE_DONE,
	CLEAR_SERVER_CACHE,
	CLEAR_SERVER_CACHE_DONE,
	CLIENT_PICTURE_FILE_NAME,
	PICTURE_ERROR,
	GET_ONLINE_DEVICE,
	SET_PC_DEVICE_FLAG,
	ONLINE_CAMERA_DEVICE_ID_TO_CLIENT,
	ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END,
	CLIENT_BIND_CAMERA,
	CLIENT_DISBIND_CAMERA,
	CLIENT_BIND_CAMERA_SUCCESS,
	CLIENT_BIND_CAMERA_FAIL,
	OPEN_MOTO_CMD,          /* 打开水泵的命令 */
	OPEN_MOTO_SUCCESS_CMD,  /* 打开成功反馈 */
	OPEN_MOTO_FAIL_CMD,     /* 打开失败反馈 */
	SET_RECORD_TIME_CMD,    /* 设置定时时间 */
	SET_RECORD_TIME_DONE_CMD,/* 设置定时结束 */
	SET_SCHEDULED_TIME_CMD, /* 设置间隔定时时间 */
	SET_RECORD_TIME_SUCCESS_CMD, /* 设置定时时间成功反馈 */
	SET_LIED_BRIGHTNESS_CMD,
	SET_TAKE_PICTURE_DELAY_TIME_CMD
};

extern char anlysis;
extern char usbOK;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
