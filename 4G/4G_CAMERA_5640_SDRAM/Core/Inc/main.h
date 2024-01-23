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


#define jpeg_buf_size   128*1024		//定义JPEG数据缓存jpeg_buf的大小(1*4M字节)
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
	CMD_JPEG_DATA_SEND_END							/* 设备通知上位机JPEG数据发送结束 */
};

enum
{
	/* 角色 */
	DEVICE_LABEL_PC = 0x00,             /* 设备标签PC端 */
	DEVICE_LABEL_CAMERA,                /* 设备标签CAMERA端 */
	DEVICE_LABEL_SERVER,                /* 设备标签SERVER端 */
	/* target */
	PC_TO_SERVER,                       /* PC发SERVER */
	PC_TO_CAMER,                        /* PC发CAMERA */
	CAMER_TO_SERVER,                    /* CAMERA发SERVER */
	CAMER_TO_PC,                        /* CAMERA发PC */
	SERVER_TO_CLIENT,                   /* SERVER发CLIENT */
	/* cmd */
	HERAT_BEAT_PACK,                    /* 心跳包 */
	SET_PC_DEVICE_FLAG,                 /* 设置PC标识 */
	SET_CAMERA_DEVICE_FLAG,             /* 设置CAMERA标识 */
	SET_CAMERA_DEVICE_ID,               /* 设置相机的ID */
	GET_ONLINE_CAMERA_DEVICE_ID,        /* 请求所有连接的camera设备ID */
	TAKE_PICTURE,                       /* 拍照命令 */
	TAKE_PICTURE_END,                   /* 拍照结束 */
	TAKE_PICTURE_ERROR,                 /* 拍照错误 */
	CAMERA_ID,                          /* 相机ID */
	SEND_CAMERA_ID_END,                 /* 发送相机ID结束 */
	CLIENT_BIND_CAMERA,                 /* 客户端绑定相机 */
	CLIENT_BIND_CAMERA_OK,              /* 客户端绑定相机成功 */
	CLIENT_DISBIND_CAMERA,              /* 客户端解绑摄像头 */
	CLIENT_DISBIND_CAMERA_OK,           /* 客户端解绑摄像头成功 */
	CLIENT_EXIST_PICTURE_FILE_NAME,     /* 客户端存在的图片文件名称 */
	CLIENT_EXIST_PICTURE_FILE_NAME_END, /* 客户端存在的图片文件名称发送结束 */
	DOWNLOAD_PICTURE,                   /* 客户端请求下载图片数据 */
	DOWNLOAD_PICTURE_END,               /* 客户端请求下载图片数据结束 */
	DOWNLOAD_PICTURE_EMPTY,             /* 客户端请求下载图片的数据为空 */
	PICTURE_TO_PC_NAME,                 /* 将图片名字发送到PC端 */
	PICTURE_TO_PC_DATA,                 /* 将图像数据发送PC端 */
	SET_RECORD_TIME,                    /* 设置定时时间 */
	SET_RECORD_TIME_END,                /* 设置定时结束 */
	SET_RECORD_TIME_SUCCESS,            /* 设置定时时间成功反馈 */
	SET_SCHEDULED_TIME,                 /* 设置间隔定时时间 */
	SET_SCHEDULED_TIME_CMD_SUCCESS,     /* 设置间隔定时时间成功反馈 */
	SAVE_PC_PICTURE,                    /* 保存由PC端发来的图片 */
	SET_TEMP_DEVICE_ID,                 /* 未绑定时，临时设置绑定ID */
	CLEAR_SERVER_CACHE,                 /* 清除服务器上的图片缓存 */
	CLEAR_SERVER_CACHE_DONE,            /* 缓存清除完毕 */
	OPEN_MOTO_CMD,                      /* 打开水泵的命令 */
	OPEN_MOTO_SUCCESS_CMD,              /* 打开成功反馈 */
	OPEN_MOTO_FAIL_CMD,                 /* 打开失败反馈 */
	PICTURE_DATA_PACK,                  /* 图像数据包 */
	PICTURE_DATA_PACK_INDEX,						/* 图像数据包编号 */
	TAKE_RGB_PICTURE_END,								/* 拍摄RGB图片结束 */
	SET_LIED_BRIGHTNESS_CMD,
	SET_TAKE_PICTURE_DELAY_TIME_CMD
};


enum sysState
{
	SYS_OK_STATE = 0,										/* 设备系统工作正常，同时也是空闲状态 */
	SYS_TEST_MODE_SATE,									/* 设备工作在测试状态 */
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
