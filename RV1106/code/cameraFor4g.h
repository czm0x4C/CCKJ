#ifndef __CAMERA_FOR_4G_H
#define __CAMERA_FOR_4G_H															    

typedef struct 
{
	char wifiName[50];						/* WIFI名字 */
	char wifiPssword[50];					/* WIFI密码 */
    uint16_t picSize;                  		/* 图片的格式 */
    unsigned int picWidth;                  /* 图片的长 */
    unsigned int picHeight;                 /* 图片的宽 */
    unsigned int scheduledDeletion;         /* 定时删除时间 */
    char deviceID[50];                      /* 设备ID */
    int jpegQuality;                        /* JPEG图像质量 */
    char serverIP[20];                  	/* ip地址 */
    unsigned int serverPort;           		/* 端口  */
	char isSetRecordTime;				    /* 记录定时组是否已经设置过了 */
    char recordTime[50][6];                 /* 定时组 */
    int recordTimeIndex;                    /* 记录固定定时的时间组 */
    unsigned int ledFlashBrightness;        /* 闪光灯亮度 */
    unsigned int takePictureDelayTime;     	/* 拍照的延时时间 */
	uint8_t sysState;						/* 系统状态 */
}_deviceInfo;
extern _deviceInfo deviceAttributeInfo;

enum frameCmd
{
	CDM_WIFI_NAME = 0,                      /* 帧携带的数据为WIFI的名称 */
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
	SET_LIED_BRIGHTNESS_CMD,
	SET_TAKE_PICTURE_DELAY_TIME_CMD
};

#endif





