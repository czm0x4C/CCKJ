#include "camera.h"
#include "uart.h"
#include "wifiConnect.h"
#include "ringBuffer.h"

#define UDP_SEND_MAX_LEN ( 60 * 1024 ) /* 设置UDP一包发送的最大数量 */

_deviceInfo deviceAttributeInfo;

nvs_handle_t nvsHandle = {0};   /* NVS读写数据句柄 */
struct tm timeinfo = {0};

TaskHandle_t cameraTaskHandle;                              /* 摄像头采任务的句柄 */
void udpReceive_task(void *pvParameters);                   /* udp数据接收任务 */
void camera_task(void *pvParameters);
/* socket使用的相关变量 */
#define HOST_IP_ADDR    "192.168.64.1"
#define PORT            (8000)
char rx_buffer[1024];
char host_ip[] = HOST_IP_ADDR;
int addr_family = 0;
int ip_protocol = 0;

struct sockaddr_in dest_addr;

int sock = 0;
/* 时间戳变量 */
unsigned long int sysStartTimeStamp = 0;
unsigned long int nowTimeStamp = 0; 
/* 系统指示灯变量 */
unsigned char netOnlineFlag;
unsigned char workFlag;

char takePictureFlag = 0;/* 拍照的变量 */
char openMotoFlag = 0;

char logMessageBuffer[260];

_RingBuffer TCP_RxRingBuffer;
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

//最新版硬件
#define CAM_PIN_PWDN    (45)
#define CAM_PIN_RESET   (39) //software reset will be performed
#define CAM_PIN_XCLK    (21)
#define CAM_PIN_SIOD    (41)
#define CAM_PIN_SIOC    (40)

#define CAM_PIN_D7      (47)
#define CAM_PIN_D6      (14)
#define CAM_PIN_D5      (13)
#define CAM_PIN_D4      (11)
#define CAM_PIN_D3      ( 9)
#define CAM_PIN_D2      ( 3)
#define CAM_PIN_D1      (46)
#define CAM_PIN_D0      (10)
#define CAM_PIN_VSYNC   (38)
#define CAM_PIN_HREF    (48)
#define CAM_PIN_PCLK    (12)
static const char *camera = "camera";

camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 8 * 1000 * 1000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format =  PIXFORMAT_JPEG,     //YUV422,GRAYSCALE,RGB565,JPEG PIXFORMAT_RGB565 PIXFORMAT_JPEG
    .frame_size = FRAMESIZE_QSXGA,       //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
    //FRAMESIZE_QVGA FRAMESIZE_QSXGA FRAMESIZE_UXGA
    .jpeg_quality = 10,                  //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,                      //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

void sntp_Init(void)/* 获取实时网络时间 */
{
    /* 使用多个sntp服务器时，请在 make menuconfig -> Component config -> LWIP -> SNTP -> Maximum bumber of NTP servers 修改为 所使用的个数 */
    sntp_stop();
    ESP_LOGI("sntp","start.");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);    // 设置单播模式
    sntp_setservername(0, "cn.ntp.org.cn");     // 设置访问服务器
    sntp_setservername(1, "ntp1.aliyun.com");
    sntp_setservername(2, "pool.ntp.org");
    sntp_setservername(3, "210.72.145.44");     // 国家授时中心服务器 IP 地址
    setenv("TZ", "CST-8", 1);                   // 东八区
    tzset();                                    // 更新本地C库时间
    sntp_init();                                // 初始化

    static unsigned char reTryCnt = 0; 
    // 延时等待SNTP初始化完成
    do {
        ESP_LOGI("sntp","wait for wifi sntp sync time---------------------");
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"获取网络时间ing");
        workLedToggle();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        reTryCnt++;
        if(reTryCnt > 20)
        {
            esp_restart();
        }
    } while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET);

    // 成功获取网络时间后停止NTP请求，不然设备重启后会造成获取网络时间失败的现象
    // 大概是服务器时根据心跳时间来删除客户端的，如果不是stop结束的客户端，下次连接服务器时就会出错
	sntp_stop();
    ESP_LOGI("sntp","sntp stop.");
}

void get_time(void)
{
    time_t now;
    time(&now);                         // 获取网络时间, 64bit的秒计数 

    localtime_r(&now, &timeinfo);       // 转换成具体的时间参数
    // ESP_LOGI("sntp", "%4d-%02d-%02d %02d:%02d:%02d week:%d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, 
    //     timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_wday); 
}

/* 读取储存的摄像头信息，进行初始化配置 */
void cameraSetConfig(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }  
    ESP_ERROR_CHECK( err );

    ESP_ERROR_CHECK(nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle));

    /* 先读NVS指定键值对中是否有数据 */
    unsigned int readNvsLen = 0;
    err = nvs_get_str(nvsHandle, "deviceState",NULL, &readNvsLen);/* 判断当前设备的状态 */
    char devideState[10];
    if(readNvsLen > 0)  /* 有数据 */
    {
        readNvsLen = 10;
        memset(devideState,0,10);
        err = nvs_get_str(nvsHandle, "deviceState",devideState, &readNvsLen); /* 读取保存在nvs中的设备状态*/
        if(strcmp(devideState,"online") == 0) /* 设备状态就绪,开始读取保存的参数数据 */
        {
            readNvsLen = 0;
            ESP_ERROR_CHECK(nvs_get_str(nvsHandle, "deviceID",NULL, &readNvsLen));/* 读取设备ID */
            if(readNvsLen > 0 )/* 有数据，那么就读取 */
            {
                readNvsLen = sizeof(deviceAttributeInfo.deviceID);
                err = nvs_get_str(nvsHandle, "deviceID",deviceAttributeInfo.deviceID, &readNvsLen); /* 读取保存在nvs中的设备ID */
                ESP_LOGI("nvs","deviceID is %s,len = %d",deviceAttributeInfo.deviceID,readNvsLen - 1);
            }

            uint32_t out_value;
            ESP_ERROR_CHECK(nvs_get_u32(nvsHandle, "setVideoTime",&out_value)); /* 读取保存在nvs中的录像时间 */
            deviceAttributeInfo.scheduledDeletion = out_value;
            ESP_LOGI("nvs","scheduledDeletion = %d.\n",deviceAttributeInfo.scheduledDeletion);

            readNvsLen = 0;
            err = nvs_get_str(nvsHandle, "picFormat",NULL, &readNvsLen);/* 读取设备ID */
            if(err != ESP_OK){
                strcpy(deviceAttributeInfo.picFormat,"PIXFORMAT_JPEG");
                camera_config.pixel_format = PIXFORMAT_JPEG;
                ESP_ERROR_CHECK(nvs_set_str(nvsHandle,"picFormat",(char *)deviceAttributeInfo.picFormat));/* 写入数据 */ 
                ESP_ERROR_CHECK(nvs_commit(nvsHandle)); /* 确认数据写入 */
            }
            else
            {
                if(readNvsLen > 0 )/* 有数据，那么就读取 */
                {
                    readNvsLen = sizeof(deviceAttributeInfo.picFormat);
                    err = nvs_get_str(nvsHandle, "picFormat",deviceAttributeInfo.picFormat, &readNvsLen); /* 读取保存在nvs中的设备ID */
                    ESP_LOGI("nvs","picFormat is %s,len = %d",deviceAttributeInfo.picFormat,readNvsLen - 1);

                    if(strcmp((char *)deviceAttributeInfo.picFormat,(char *)"PIXFORMAT_JPEG") == 0)
                    {
                        camera_config.pixel_format = PIXFORMAT_JPEG;
                    }
                    else
                    {
                        camera_config.pixel_format = PIXFORMAT_JPEG;
                    }
                }
            }

            err = nvs_get_u32(nvsHandle, "picQuality",&out_value); /* 读取保存在nvs中的JPEG图像质量 */
            if(err != ESP_OK){
                deviceAttributeInfo.jpegQuality = 20;
                camera_config.jpeg_quality = deviceAttributeInfo.jpegQuality;
                uint32_t write_value = deviceAttributeInfo.jpegQuality;
                ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"picQuality",write_value));/* 写入数据 */ 
                ESP_ERROR_CHECK(nvs_commit(nvsHandle)); /* 确认数据写入 */
            }
            else
            {
                camera_config.jpeg_quality = out_value;
                ESP_LOGI("nvs","jpegQuality = %d.\n",camera_config.jpeg_quality);
            }
            
            
            err = nvs_get_u32(nvsHandle, "picSize",&out_value); /* 读取保存在nvs中的图像大小 */
            if(err != ESP_OK){
                camera_config.frame_size = FRAMESIZE_UXGA;
                uint32_t write_value = camera_config.frame_size;
                ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"picSize",write_value));/* 写入数据 */ 
                ESP_ERROR_CHECK(nvs_commit(nvsHandle)); /* 确认数据写入 */
            }
            else
            {
                camera_config.frame_size = out_value;
                ESP_LOGI("nvs","frame_size = %d.\n",camera_config.frame_size);
            }

            ESP_ERROR_CHECK(nvs_get_u32(nvsHandle,"setLedBri",&out_value));
            deviceAttributeInfo.ledFlashBrightness = out_value;

            ESP_ERROR_CHECK(nvs_get_u32(nvsHandle,"setDelayTime",&out_value));
            deviceAttributeInfo.takePictureDelayTime = out_value;

            ESP_LOGI("UART","灯的亮度为 = %d",deviceAttributeInfo.ledFlashBrightness);
            ESP_LOGI("UART","拍照延时 = %d",deviceAttributeInfo.takePictureDelayTime);

            nvs_close(nvsHandle);

            err = nvs_open("wifiInfo", NVS_READWRITE, &nvsHandle);
            if (err != ESP_OK) 
            {
                ESP_LOGI("nvs","Error (%s) opening NVS handle!", esp_err_to_name(err));
            }

            readNvsLen = 0;
            err = nvs_get_str(nvsHandle, "udpIP",NULL, &readNvsLen);/* 读取udp地址 */
            if(readNvsLen > 0 )/* 有数据，那么就读取 */
            {
                readNvsLen = sizeof(deviceAttributeInfo.UDP_serverIP);
                err = nvs_get_str(nvsHandle, "udpIP",deviceAttributeInfo.UDP_serverIP, &readNvsLen); /* 读取保存在nvs中的udp ip */
                ESP_LOGI("nvs","UDP_serverIP is %s,len = %d",deviceAttributeInfo.UDP_serverIP,readNvsLen - 1);
            }
            
            out_value = 0;
            err = nvs_get_u32(nvsHandle, "udpPORT",&out_value);/* 读取udp地址 */
            deviceAttributeInfo.UDP_serverPort = out_value;
            ESP_LOGI("nvs","UDP_serverPort = %d.",deviceAttributeInfo.UDP_serverPort);
            nvs_close(nvsHandle);
            
        }
        else
        {
            strcpy(devideState,"");/* 写入空数据 */
            err = nvs_set_str(nvsHandle,"deviceState",devideState);/* 写入数据 */
            if(err==ESP_OK)
            {
                ESP_LOGI("nvs","set deviceID success!\n");
            }
            else
            {
                ESP_LOGI("nvs","set deviceID fail!\n");
            }  
            err = nvs_commit(nvsHandle); /* 确认数据写入 */
            if(err != ESP_OK)
            {
                ESP_LOGI("nvs","nvs_commit Failed!");
            }
            else
            {
                ESP_LOGI("nvs","nvs_commit Done!");
            }
        }
    }
    else        /* 读取设备状态，未就绪 */
    {
        // sntp_Init(); /* 利用sntp定时器获取时间 */
        // get_time();/* 获取时间 */
        strcpy(devideState,"online");
        err = nvs_set_str(nvsHandle,"deviceState",devideState);/* 写入设备状态，就绪 */
        if(err==ESP_OK)
        {
            ESP_LOGI("nvs","set deviceState success!\n");
        }
        else
        {
            ESP_LOGI("nvs","set deviceState fail!\n");
        }  
        memset(deviceAttributeInfo.deviceID,0,sizeof(deviceAttributeInfo.deviceID));
        sprintf(deviceAttributeInfo.deviceID,"%04d",1010);
        ESP_LOGI("nvs","set data %s .\n",deviceAttributeInfo.deviceID);

        err = nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle);
        err = nvs_set_str(nvsHandle,"deviceID",deviceAttributeInfo.deviceID);/* 写入设备ID */
        if(err==ESP_OK)
        {
            ESP_LOGI("nvs","set deviceID success!\n");
        }
        else
        {
            ESP_LOGI("nvs","set deviceID fail!\n");
        }  

        deviceAttributeInfo.scheduledDeletion = 24 * 60;
        err = nvs_set_u32(nvsHandle,"setVideoTime",deviceAttributeInfo.scheduledDeletion);/* 写入录像时间 */  
        if(err==ESP_OK)
        {
            ESP_LOGI("nvs","set setVideoTime success!\n");
        }
        else
        {
            ESP_LOGI("nvs","set setVideoTime fail!\n");
        }  

        ESP_ERROR_CHECK(nvs_set_blob(nvsHandle,"recordTime",(char *)deviceAttributeInfo.recordTime,240));/* 写入固定定时时间列表 */

        deviceAttributeInfo.ledFlashBrightness = 0;

        ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"setLedBri",deviceAttributeInfo.ledFlashBrightness));/* 写入闪光灯亮度 */ 

        deviceAttributeInfo.takePictureDelayTime = 0;

        ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"setDelayTime",deviceAttributeInfo.takePictureDelayTime));/* 写入拍照延时 */  

        err = nvs_commit(nvsHandle); /* 确认数据写入 */
        if(err != ESP_OK)
        {
            ESP_LOGI("nvs","nvs_commit Failed!");
        }
        else
        {
            ESP_LOGI("nvs","nvs_commit Done!");
        }
        
        nvs_close(nvsHandle);
        esp_restart();/* 重启设备 */
    }
    
    nvs_close(nvsHandle);
}

void getDeviceInfo(void)
{
    switch(camera_config.pixel_format)/* 判断设置的图片格式 */
    {
        case PIXFORMAT_RGB565:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_RGB565");
            break;
        case PIXFORMAT_YUV422:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_YUV422");
            break;            
        case PIXFORMAT_YUV420:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_YUV420");
            break;
        case PIXFORMAT_GRAYSCALE:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_GRAYSCALE");
            break;
        case PIXFORMAT_JPEG:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_JPEG");
            break;
        case PIXFORMAT_RGB888:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            sprintf(deviceAttributeInfo.picFormat,"%s","PIXFORMAT_RGB888");
            break;
        default:
            memset(deviceAttributeInfo.picFormat,0,20);/* 清空数组，长度20 */
            break;
    }
    switch(camera_config.frame_size)/* 判断设置的图片大小 */
    {
        case FRAMESIZE_QVGA:
            deviceAttributeInfo.picWidth  = 320;
            deviceAttributeInfo.picHeight = 240;
            break;
        case FRAMESIZE_VGA:
            deviceAttributeInfo.picWidth  = 680;
            deviceAttributeInfo.picHeight = 480;
            break;    
        case FRAMESIZE_UXGA:
            deviceAttributeInfo.picWidth  = 1600;
            deviceAttributeInfo.picHeight = 1200;
            break;    
        case FRAMESIZE_QSXGA:
            deviceAttributeInfo.picWidth  = 2560;
            deviceAttributeInfo.picHeight = 1920;
            break;          
        default:
            break;
    }
    deviceAttributeInfo.jpegQuality = camera_config.jpeg_quality;

    /* 读取定时时间 */
    ESP_ERROR_CHECK(nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle));
    size_t readBlobSize = 300;
    ESP_ERROR_CHECK(nvs_get_blob(nvsHandle,"recordTime",(char *)deviceAttributeInfo.recordTime,&readBlobSize));

    nvs_close(nvsHandle);
}

void writeDeviceInfo(void)
{
    writeWifiInfo(wifiName,wifiPassword,(unsigned char *)deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort,(unsigned char *)"ok");

    ESP_ERROR_CHECK(nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle));
    ESP_ERROR_CHECK(nvs_set_str(nvsHandle,"deviceID",deviceAttributeInfo.deviceID));/* 写入设备ID */

    char eraseTempBuffer[50][6];
    ESP_ERROR_CHECK(nvs_set_blob(nvsHandle,"recordTime",eraseTempBuffer,300));/* 写入定时时间 */
    ESP_ERROR_CHECK(nvs_set_blob(nvsHandle,"recordTime",(char *)deviceAttributeInfo.recordTime,300));/* 写入定时时间 */

    deviceAttributeInfo.recordTimeIndex = 0;

    ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"setVideoTime",deviceAttributeInfo.scheduledDeletion));/* 写入间隔定时时间 */  

    ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"setLedBri",deviceAttributeInfo.ledFlashBrightness));/* 写入闪光灯亮度 */ 

    ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"setDelayTime",deviceAttributeInfo.takePictureDelayTime));/* 写入拍照延时 */  

    nvs_close(nvsHandle);
    setCameraPara();
}

void setCameraPara(void)
{
    ESP_ERROR_CHECK(nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle));
    uint32_t write_value = camera_config.frame_size;
    ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"picSize",write_value));/* 写入数据 */ 
    ESP_ERROR_CHECK(nvs_commit(nvsHandle)); /* 确认数据写入 */
    nvs_close(nvsHandle);

    ESP_ERROR_CHECK(nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle));
    write_value = camera_config.jpeg_quality;
    ESP_ERROR_CHECK(nvs_set_u32(nvsHandle,"picQuality",write_value));/* 写入数据 */ 
    ESP_ERROR_CHECK(nvs_commit(nvsHandle)); /* 确认数据写入 */
    nvs_close(nvsHandle);
}

void showEspInfo(void)
{
    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"摄像头图像格式：%s",deviceAttributeInfo.picFormat);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"摄像头图像尺寸：%d*%d",deviceAttributeInfo.picHeight,deviceAttributeInfo.picWidth);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"摄像头图像质量：%d",deviceAttributeInfo.jpegQuality);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"摄像头ID：%s",deviceAttributeInfo.deviceID);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"摄像头定时时间：%d",deviceAttributeInfo.scheduledDeletion);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"连接服务器地址：%s，服务器端口：%d",deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    for(int i=0;i<48;i++)
    {
        // deviceAttributeInfo.recordTime[i];
        ESP_LOGI("info","%s",deviceAttributeInfo.recordTime[i]);
    }
}

void camera_task(void *pvParameters)
{
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:创建摄像头任务");
    getDeviceInfo();
  
    static unsigned int Time1 = 0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ;
    static unsigned int Time2 = 0;
    static unsigned int Time3 = 0;
    static unsigned int Time4 = 0;
    // static unsigned int Time5 = 0;
    // static unsigned int Time6 = 0;
    static unsigned int Time7 = 0;
    static unsigned int Time8 = 0;

    Time1 = esp_log_timestamp();
    Time2 = Time1;
    Time3 = Time1;
    Time4 = Time1;
    Time7 = Time1;
    Time8 = Time1;

    char frameBuffer[100];
    unsigned int sendTcpDataLen = 0;

    camera_fb_t *pic;
    ESP_LOGI("camera","开始设置摄像头");
    esp_camera_deinit();                                /* 复位摄像头的硬件设置 */
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:重置摄像头");

    // camera_config.pixel_format = PIXFORMAT_JPEG;//PIXFORMAT_YUV422 PIXFORMAT_RGB565 PIXFORMAT_JPEG
    // camera_config.frame_size = FRAMESIZE_UXGA;

    // camera_config.jpeg_quality = 10;

    if(CAM_PIN_PWDN != -1){
        esp_rom_gpio_pad_select_gpio(CAM_PIN_PWDN);
        gpio_set_direction(CAM_PIN_PWDN, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出

        gpio_set_level(CAM_PIN_PWDN, 1);
    }

    esp_err_t err = esp_camera_init(&camera_config);    /* 按照既定参数设置摄像头 */
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:设置摄像头");
    ESP_LOGI("camera","设置摄像头结束");

    if (err != ESP_OK)
    {
        memset(logMessageBuffer,0,sizeof(logMessageBuffer));
        sprintf(logMessageBuffer,"ESP:camera_err = %d",err);
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

        memset(logMessageBuffer,0,sizeof(logMessageBuffer));
        sprintf(logMessageBuffer,"ESP:设置摄像头失败，5s后重启");
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

        vTaskDelay(5000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:设置摄像头成功");

    memset(frameBuffer,0,100);/* 清空数组 */
    sendTcpDataLen = 1;
    frameBuffer[0] = sendTcpDataLen;
    frameBuffer[1] = sendTcpDataLen>>8;
    frameBuffer[2] = sendTcpDataLen>>16;
    frameBuffer[3] = sendTcpDataLen>>24;

    frameBuffer[4] = SET_CAMERA_DEVICE_FLAG;

    int tcpSendErr = 0;

    tcpSendErr = send(sock,frameBuffer,5, 0);
    if(tcpSendErr <= 0)
    {
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:TCP发送错误");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }

    memset(frameBuffer,0,100);/* 清空数组 */
    sendTcpDataLen = 1 + strlen(deviceAttributeInfo.deviceID);
    frameBuffer[0] = sendTcpDataLen;
    frameBuffer[1] = sendTcpDataLen>>8;
    frameBuffer[2] = sendTcpDataLen>>16;
    frameBuffer[3] = sendTcpDataLen>>24;

    frameBuffer[4] = SET_CAMERA_DEVICE_ID;

    memcpy(&frameBuffer[5],&deviceAttributeInfo.deviceID,strlen(deviceAttributeInfo.deviceID));
    tcpSendErr = send(sock,frameBuffer,5+strlen(deviceAttributeInfo.deviceID), 0);
    if(tcpSendErr <= 0)
    {
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:TCP发送错误");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    } 

    while(1)
    {
        // ESP_LOGI(camera, "Taking picture...");
        if(true)/* 接收的图像数据正常 */
        {
            // use pic->buf to access the image


            Time2 = esp_log_timestamp();
            if((Time2 - Time1) > 5000)
            {
                // send(sock,pic->buf,pic->len, 0);
                /* 发送图片包 */
                // memset(frameBuffer,0,100);/* 清空数组 */
                // sendTcpDataLen = pic->len + 1;
                // frameBuffer[0] = sendTcpDataLen;
                // frameBuffer[1] = sendTcpDataLen>>8;
                // frameBuffer[2] = sendTcpDataLen>>16;
                // frameBuffer[3] = sendTcpDataLen>>24;

                // frameBuffer[4] = PICTURE_DATA;
                
                // send(sock,frameBuffer,5, 0);/* 发送帧头 */
                // send(sock,pic->buf,pic->len, 0);/* 发送数据包 */
                // get_time();
                Time1 = Time2;
            }
/*******************************************************************************心跳包**********************************************************************************************/
            Time4 = esp_log_timestamp();
            if((Time4 - Time3) > 1000)
            {
                /* 发送TCP心跳包 */
                memset(frameBuffer,0,100);/* 清空数组 */
                sendTcpDataLen = 1;
                frameBuffer[0] = sendTcpDataLen;
                frameBuffer[1] = sendTcpDataLen>>8;
                frameBuffer[2] = sendTcpDataLen>>16;
                frameBuffer[3] = sendTcpDataLen>>24;

                frameBuffer[4] = HERAT_BEAT_PACK;
                
                tcpSendErr = send(sock,frameBuffer,5, 0);
                if(tcpSendErr <= 0)esp_restart();
                Time3 = Time4;
            }
/*******************************************************************************固定定时判断**********************************************************************************************/
            static unsigned char timeArrive = 0;
            char timeCmp[6];
            get_time();
            sprintf(timeCmp,"%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min);
            for(int i=0;i<50;i++)
            {
                if(strcmp(timeCmp,deviceAttributeInfo.recordTime[i]) == 0 && timeArrive != i)
                {
                    ESP_LOGI("camera","定时时间到%02d:%02d",timeinfo.tm_hour,timeinfo.tm_min);
                    takePictureFlag = 1;
                    timeArrive = i;
                }
            }
/*******************************************************************************间隔定时判断**********************************************************************************************/
            if(deviceAttributeInfo.scheduledDeletion != 0)/* 定时有效 */
            {
                Time8 = esp_log_timestamp();
                if((Time8 - Time7) > (deviceAttributeInfo.scheduledDeletion * 1000 * 60))
                {
                    takePictureFlag = 1;
                    Time7 = Time8;
                }
            }
/*******************************************************************************拍照**********************************************************************************************/
            if(takePictureFlag)
            {
                vTaskDelay(deviceAttributeInfo.takePictureDelayTime * 1000 /portTICK_PERIOD_MS);
                flashLedOn();
                
                pic = esp_camera_fb_get();
                esp_camera_fb_return(pic);

                pic = esp_camera_fb_get();
                
                char tryCnt = 0;

                while(pic == NULL)
                {
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:图片内容错误");
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    pic = esp_camera_fb_get();
                    tryCnt++;
                    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
                    sprintf(logMessageBuffer,"ESP:尝试第 = %d",tryCnt);
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    if(tryCnt>50)
                    {
                        memset(frameBuffer,0,100);/* 清空数组 */
                        sendTcpDataLen = 1;
                        frameBuffer[0] = sendTcpDataLen;
                        frameBuffer[1] = sendTcpDataLen>>8;
                        frameBuffer[2] = sendTcpDataLen>>16;
                        frameBuffer[3] = sendTcpDataLen>>24;

                        frameBuffer[4] = PICTURE_ERROR;
                        tcpSendErr = send(sock,frameBuffer,5, 0);/* 发送帧头 */
                        if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:3 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}

                        flashLedOff();

                        break;
                    }
                }

                if(pic != NULL)
                {
                    ESP_LOGI(camera, "Picture taken! Its size was: %zu bytes,width: %zu,height: %zu", 
                        pic->len,pic->width,pic->height);
                    
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:正在发送图片");
                                    /* 发送图片包 */
                    memset(frameBuffer,0,100);/* 清空数组 */
                    sendTcpDataLen = pic->len + 1;
                    frameBuffer[0] = sendTcpDataLen;
                    frameBuffer[1] = sendTcpDataLen>>8;
                    frameBuffer[2] = sendTcpDataLen>>16;
                    frameBuffer[3] = sendTcpDataLen>>24;

                    frameBuffer[4] = PICTURE_DATA;
                    
                    tcpSendErr = send(sock,frameBuffer,5, 0);/* 发送帧头 */
                    if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:1 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}
                    tcpSendErr = send(sock,pic->buf,pic->len, 0);/* 发送数据包 */
                    if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:2 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}

                    flashLedOff();
                    memset(frameBuffer,0,100);/* 清空数组 */
                    sendTcpDataLen = 1;
                    frameBuffer[0] = sendTcpDataLen;
                    frameBuffer[1] = sendTcpDataLen>>8;
                    frameBuffer[2] = sendTcpDataLen>>16;
                    frameBuffer[3] = sendTcpDataLen>>24;

                    frameBuffer[4] = CAMERA_TAKE_PICTURE_DONE;
                    
                    tcpSendErr = send(sock,frameBuffer,5, 0);/* 发送帧头 */
                    if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:3 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}
                    ESP_LOGI("camera","拍照完成");
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:拍照完成");
                    esp_camera_fb_return(pic);
                }
                else
                {
                    flashLedOff();
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:图片内容错误");
                    esp_camera_fb_return(pic);
                }
                takePictureFlag = 0;
            }
/*******************************************************************************外部电平检测**********************************************************************************************/
            static char keyFlag = 0;
            if(keyValue(KEY_IO) == 0 && keyFlag == 0)/* 吸合检测到低电平 */
            {
                takePictureFlag = 1;
                keyFlag = 1;
                espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:外部触发拍照");
                motoOff();
                espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:水泵关闭");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            else if(keyValue(KEY_IO) == 1 && keyFlag == 1)/* 断开检测到高电平 */
            {
                keyFlag = 0;
            }
/*********************************************************************************继电器控制****************************************************************************************************/
            if(openMotoFlag == 1)
            {
                motoOn();
                memset(frameBuffer,0,100);/* 清空数组 */
                sendTcpDataLen = 1;
                frameBuffer[0] = sendTcpDataLen;
                frameBuffer[1] = sendTcpDataLen>>8;
                frameBuffer[2] = sendTcpDataLen>>16;
                frameBuffer[3] = sendTcpDataLen>>24;

                frameBuffer[4] = OPEN_MOTO_SUCCESS_CMD;
                tcpSendErr = send(sock,frameBuffer,5, 0);/* 发送帧头 */
                if(tcpSendErr <= 0){espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:3 TCP发送错误");vTaskDelay(1000 / portTICK_PERIOD_MS);esp_restart();}

                espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:水泵打开");

                // vTaskDelay(2000 / portTICK_PERIOD_MS);
                // motoOff();
                // espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:水泵关闭");
                openMotoFlag = 0;
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        else
        {
            ESP_LOGI("camera","error");
            espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:读取摄像头失败！");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            workFlag = 0;
        }
    }
}

void led_task(void *pvParameters)
{
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:创建指示灯任务");

    while(1)
    {
        if(netOnlineFlag)
        {
            netLedOn();
        }
        else
        {
            netLedOff();
        }
        if(workFlag)
        {
            workLedOn();
        }
        else
        {
            workLedOff();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
}

void tcpReceive_task(void *pvParameters)
{
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:创建TCP数据接收任务");
    RingBuffer_Init(&TCP_RxRingBuffer);
    while(1)
    {
        ESP_LOGI("TCP","接收指令");
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occurred during receiving
        if (len < 0) {
            ESP_LOGE("camera", "recv failed: errno %d", errno);
            esp_restart();
            break;
        }
        else 
        {
            // rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            // if(len>4)
            // {
            //     unsigned int recTcpDataLen = rx_buffer[0] | rx_buffer[1]<<8 |
            //                                 rx_buffer[2]<<16 | rx_buffer[3]<<24;
            //     if((recTcpDataLen == 1) && (rx_buffer[4] == CAMERA_TAKE_PICTURE))
            //     {
            //         takePictureFlag = 1;
            //         espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到拍照指令");
            //         ESP_LOGI("TCP","收到拍照指令");
            //     }                
            //     if((recTcpDataLen == 1) && (rx_buffer[4] == OPEN_MOTO_CMD))
            //     {
            //         espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到打开水泵指令");
            //         openMotoFlag = 1;
            //         ESP_LOGI("TCP","收到打开水泵指令");
            //     }
            //     if((recTcpDataLen == 1) && (rx_buffer[4] == SET_RECORD_TIME_CMD))
            //     {
            //         memcpy((char *)deviceAttributeInfo.recordTime, &rx_buffer[5],len - 4);
            //         ESP_LOGI("TCP","收到数据:%s",(char *)deviceAttributeInfo.recordTime);
            //         // espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:定时时间");
            //     }
            // }
            /* 收到数据，全部写入缓冲区 */
            WriteBytes(&TCP_RxRingBuffer,(unsigned char *)rx_buffer,len);

            unsigned char *FrameData;                                               /*存储解析帧的空间*/
		    unsigned short allFrameDataLen = TCP_RxRingBuffer.Lenght;               /* 获取缓冲区中的存储的数据长度 */
            int dataIndex = 0;                                                      /* 数组中的偏移位置 */

            if(xPortGetFreeHeapSize() >= allFrameDataLen)                           /* 判断剩余内存是否大于申请内存 */
            {
                FrameData = (unsigned char *)pvPortMalloc(allFrameDataLen);	        /*为读取缓冲区中的所有数据申请内存*/
                if(ReadBytes(&TCP_RxRingBuffer,FrameData,allFrameDataLen) == 1)		/*读取缓冲区中的所有数据*/
                {
                    while(allFrameDataLen - dataIndex >= 4)/* 仍然可以得到数据长度 */
                    {
                        unsigned int frameDataLen = (int)FrameData[dataIndex + 0] |
                                                    (int)FrameData[dataIndex + 1] <<  8 |
                                                    (int)FrameData[dataIndex + 2] << 16 |
                                                    (int)FrameData[dataIndex + 3] << 24;/* 得到长度数据 */

                        if(allFrameDataLen - dataIndex - 4 >= frameDataLen)/* 能得到数据内容 */
                        {   
                            dataIndex += 4;
                            switch(FrameData[dataIndex])
                            {
                                case CAMERA_TAKE_PICTURE:/* 拍照 */
                                {
                                    takePictureFlag = 1;
                                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到拍照指令");
                                    break;
                                }
                                case OPEN_MOTO_CMD:
                                {
                                    openMotoFlag = 1;
                                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到打开水泵指令");
                                    break;
                                }
                                case SET_RECORD_TIME_CMD:/* 设置固定定时 */
                                {
                                    dataIndex++;
                                    if(deviceAttributeInfo.recordTimeIndex > 50)
                                    {
                                        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:超过最大设置定时分组，请删除重试");
                                        break;
                                    }
                                    memcpy((char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex],
                                                &FrameData[dataIndex],5);
                                    deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex][5] = '\0';
                                    
                                    ESP_LOGI("TCP","收到数据:%s",(char *)deviceAttributeInfo.recordTime[deviceAttributeInfo.recordTimeIndex]);
                                    deviceAttributeInfo.recordTimeIndex++;
                                    
                                    deviceAttributeInfo.scheduledDeletion = 0;/* 固定定时时，间隔定时时间设置无效 */
                                    dataIndex--;
                                    // espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:定时时间");
                                    break;
                                }
                                case SET_RECORD_TIME_DONE_CMD:
                                {
                                    ESP_LOGI("TCP","定时设置结束");
                                    deviceAttributeInfo.recordTimeIndex = 0;
                                    writeDeviceInfo();/* 写入数据到NVS中保存 */
                                    break;
                                }
                                case SET_SCHEDULED_TIME_CMD:
                                {
                                    ESP_LOGI("TCP","间隔定时");
                                    deviceAttributeInfo.scheduledDeletion = (unsigned short)FrameData[dataIndex + 1] | 
                                                                            (unsigned short)FrameData[dataIndex + 2]<<8;

                                    for(int i=0;i<50;i++)/* 间隔定时时清除固定定时设置的时间 */
                                    {
                                        memset(deviceAttributeInfo.recordTime[i],0,6);
                                    }
                                    ESP_LOGI("TCP","scheduledDeletion = %d",deviceAttributeInfo.scheduledDeletion);
                                    break;
                                }
                                case SET_LIED_BRIGHTNESS_CMD:   /* 设置闪光灯亮度 */
                                {
                                    ESP_LOGI("TCP","设置闪光灯亮度");
                                    deviceAttributeInfo.ledFlashBrightness = (unsigned short)FrameData[dataIndex + 1] | 
                                                                            (unsigned short)FrameData[dataIndex + 2]<<8;

                                    ESP_LOGI("TCP","ledFlashBrightness = %d",deviceAttributeInfo.ledFlashBrightness);
                                    break;
                                }
                                case SET_TAKE_PICTURE_DELAY_TIME_CMD:    /* 设置拍照延时时间 */
                                {
                                    ESP_LOGI("TCP","拍照延时时间");
                                    deviceAttributeInfo.takePictureDelayTime = (unsigned short)FrameData[dataIndex + 1] | 
                                                                            (unsigned short)FrameData[dataIndex + 2]<<8;

                                    ESP_LOGI("TCP","takePictureDelayTime = %d",deviceAttributeInfo.takePictureDelayTime);
                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }
                            dataIndex += frameDataLen;
                        }
                        else/* 有数据长度但是没有数据内容,继续接受数据,下一次解析 */
                        {
                            WriteBytes(&TCP_RxRingBuffer,&FrameData[dataIndex],allFrameDataLen - dataIndex);/* 写入剩余数据 */
                            break;
                        }
                    }
                }
                vPortFree(FrameData);  //释放内存
            }
        }
    }
}

void appInit(void)
{
    netOnlineFlag = 1;
    workFlag = 0;

    /* 当wifi连接正常后才开始socket设置和连接 */
    /* 初始化TCP */

    dest_addr.sin_addr.s_addr = inet_addr(deviceAttributeInfo.UDP_serverIP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(deviceAttributeInfo.UDP_serverPort);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:创建TCPsocket");
    sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:无法创建TCPsocket,请重新配置或重启");
        ESP_LOGE("TCP", "Unable to create socket: errno %d", errno);
    }
    ESP_LOGI("TCP", "Socket created, connecting to %s:%d", deviceAttributeInfo.UDP_serverIP, deviceAttributeInfo.UDP_serverPort);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:开始TCP连接");

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    
    if (err != 0) 
    {
        ESP_LOGE("TCP", "Socket unable to connect: errno %d", errno);
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:TCP连接失败，请检查或更新配置！将在10s后重启尝试连接！");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    else
    {
        ESP_LOGI("TCP", "Successfully connected");
        /* 此时已经连接上TCP服务器 */
        // ESP_LOGI("TCP", "MEM = %ld",xPortGetFreeHeapSize()  );

        xTaskCreate(camera_task, "camera_task", 1024 * 60, NULL, 2, &cameraTaskHandle); 

        xTaskCreate(led_task, "led_task", 1024 * 10, NULL, 3, NULL); 

        xTaskCreate(tcpReceive_task, "tcpReceive_task", 1024 * 10, NULL, 3, NULL); 

        // ESP_LOGI("TCP", "MEM = %ld",xPortGetFreeHeapSize()  );
        
        workFlag = 1;
    }

}