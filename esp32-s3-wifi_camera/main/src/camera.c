#include "camera.h"
#include "uart.h"
#include "wifiConnect.h"

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
char rx_buffer[128];
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

char logMessageBuffer[260];

    enum {HERAT_BEAT_PACK = 0x00,PICTURE_DATA,DOWNLOAD_PICTURE,EMPTY,
          PICTURE_TO_CLIENT_NAME,PICTURE_TO_CLIENT_DATA,PICTURE_TO_CLIENT_END,
         TAKE_PICTURE,SET_CAMERA_DEVICE_FLAG,SET_CAMERA_DEVICE_ID,CAMERA_TAKE_PICTURE,CAMERA_TAKE_PICTURE_DONE,
         CLEAR_SERVER_CACHE,CLEAR_SERVER_CACHE_DONE,
         CLIENT_PICTURE_FILE_NAME,
          PICTURE_ERROR,
         GET_ONLINE_DEVICE,SET_PC_DEVICE_FLAG,
          ONLINE_CAMERA_DEVICE_ID_TO_CLIENT,ONLINE_CAMERA_DEVICE_LIST_TO_CLIENT_END,
         CLIENT_BIND_CAMERA};

//第一版硬件
// #define CAM_PIN_PWDN    (45)
// #define CAM_PIN_RESET   (48) //software reset will be performed
// #define CAM_PIN_XCLK    (47)
// #define CAM_PIN_SIOD    (18)
// #define CAM_PIN_SIOC    ( 8)

// #define CAM_PIN_D7      (12)
// #define CAM_PIN_D6      (11)
// #define CAM_PIN_D5      (10)
// #define CAM_PIN_D4      ( 9)
// #define CAM_PIN_D3      (46)
// #define CAM_PIN_D2      ( 3)
// #define CAM_PIN_D1      (20)
// #define CAM_PIN_D0      (19)
// #define CAM_PIN_VSYNC   (13)
// #define CAM_PIN_HREF    (21)
// #define CAM_PIN_PCLK    (14)

// //第二版硬件
// #define CAM_PIN_PWDN    (3)    //3
// #define CAM_PIN_RESET   (19) //19
// #define CAM_PIN_XCLK    (10)    //10
// #define CAM_PIN_SIOD    (18)
// #define CAM_PIN_SIOC    ( 8)

// #define CAM_PIN_D7      (9)    //9
// #define CAM_PIN_D6      (11)
// #define CAM_PIN_D5      (12)    //12
// #define CAM_PIN_D4      (14)    //14
// #define CAM_PIN_D3      (47)    //47
// #define CAM_PIN_D2      (45)   //pwdn 45
// #define CAM_PIN_D1      (48)    //rest 48
// #define CAM_PIN_D0      (21)    //21
// #define CAM_PIN_VSYNC   (20)    //20
// #define CAM_PIN_HREF    (46)    //46
// #define CAM_PIN_PCLK    (13)    //13
// //CCKJ
// #define CAM_PIN_PWDN    (38)    //3
// #define CAM_PIN_RESET   (40) //19
// #define CAM_PIN_XCLK    (48)    //10
// #define CAM_PIN_SIOD    (42)
// #define CAM_PIN_SIOC    (41)

// #define CAM_PIN_D7      (45)    //9
// #define CAM_PIN_D6      (47)
// #define CAM_PIN_D5      (21)    //12
// #define CAM_PIN_D4      (13)    //14
// #define CAM_PIN_D3      (11)    //47
// #define CAM_PIN_D2      (9)   //pwdn 45
// #define CAM_PIN_D1      (10)    //rest 48
// #define CAM_PIN_D0      (12)    //21
// #define CAM_PIN_VSYNC   (39)    //20
// #define CAM_PIN_HREF    (0)    //46
// #define CAM_PIN_PCLK    (14)    //13

//第二版硬件
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
        // sprintf(deviceAttributeInfo.deviceID,"%04d%02d%02d%02d%02d%02d",  timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        //                                                              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);/* 年月日时分秒 */
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

        deviceAttributeInfo.scheduledDeletion = 3600 * 24 * 15;
        err = nvs_set_u32(nvsHandle,"setVideoTime",deviceAttributeInfo.scheduledDeletion);/* 写入录像时间 */  
        if(err==ESP_OK)
        {
            ESP_LOGI("nvs","set setVideoTime success!\n");
        }
        else
        {
            ESP_LOGI("nvs","set setVideoTime fail!\n");
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
}

void writeDeviceInfo(void)
{
    writeWifiInfo(wifiName,wifiPassword,(unsigned char *)deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort,(unsigned char *)"ok");

    esp_err_t err = nvs_open("deviceInfo", NVS_READWRITE, &nvsHandle);
    err = nvs_set_str(nvsHandle,"deviceID",deviceAttributeInfo.deviceID);/* 写入设备ID */
    if(err==ESP_OK)
    {
        memset(logMessageBuffer,0,sizeof(logMessageBuffer));
        sprintf(logMessageBuffer,"ESP:设备DI = %s",deviceAttributeInfo.deviceID);
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);
    }
    else
    {
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"设备ID设置错误!");
    }  

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

void camera_task(void *pvParameters)
{
    getDeviceInfo();
  
    static unsigned int Time1 = 0                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ;
    static unsigned int Time2 = 0;
    static unsigned int Time3 = 0;
    static unsigned int Time4 = 0;
    char frameBuffer[100];
    unsigned int sendTcpDataLen = 0;

    camera_fb_t *pic;
    ESP_LOGI("camera","开始设置摄像头");
    esp_camera_deinit();                                /* 复位摄像头的硬件设置 */
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:重置摄像头");

    camera_config.pixel_format = PIXFORMAT_JPEG;//PIXFORMAT_YUV422 PIXFORMAT_RGB565 PIXFORMAT_JPEG
    camera_config.frame_size = FRAMESIZE_UXGA;

    camera_config.jpeg_quality = 10;

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
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:设置摄像头成功");
    
    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"ESP:camera_config.frame_size = %d,camera_config.jpeg_quality = %d",camera_config.frame_size,camera_config.jpeg_quality);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

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


    setKeyValue(1);

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
                Time1 = Time2;
            }

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

            if(takePictureFlag)
            {
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
                    ESP_LOGI(camera, "sendPicture~");
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

            static char keyFlag = 0;
            if(keyValue(18) == 1 && keyFlag == 0)/* 断开检测到高电平 */
            {
                // takePictureFlag = 1;
                keyFlag = 1;
                espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:外部高电平触发拍照");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            else if(keyValue(18) == 0 && keyFlag == 1)/* 等待联通 */
            {
                keyFlag = 0;
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
    LED_Init();/* led初始化 */
    ESP_LOGE("LED", "init");
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
    ESP_LOGI("camera","任务开始");
    while(1)
    {
        ESP_LOGI("camera","接收指令");
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        ESP_LOGI("camera","OK");
        // Error occurred during receiving
        if (len < 0) {
            ESP_LOGE("camera", "recv failed: errno %d", errno);
            esp_restart();
            break;
        }
        else 
        {
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            if(len>4)
            {
                unsigned int recTcpDataLen = rx_buffer[0] | rx_buffer[1]<<8 |
                                            rx_buffer[2]<<16 | rx_buffer[3]<<24;
                if((recTcpDataLen == 1) && (rx_buffer[4] == CAMERA_TAKE_PICTURE))
                {
                    takePictureFlag = 1;
                    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:收到拍照指令");
                    ESP_LOGI("camera","收到拍照指令");
                }                
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
    
    memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    sprintf(logMessageBuffer,"ESP:IP = %s,port = %d",deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort);
    espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
    
    if (err != 0) 
    {
        ESP_LOGE("TCP", "Socket unable to connect: errno %d", errno);
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:TCP连接失败,请重新配置或重启");
    }
    else
    {
        ESP_LOGI("TCP", "Successfully connected");
        /* 此时已经连接上TCP服务器 */
        ESP_LOGI("TCP", "MEM = %ld",xPortGetFreeHeapSize()  );

        // /* 创建摄像头任务 */
        espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:创建摄像头任务");

        xTaskCreate(camera_task, "camera_task", 1024 * 60, NULL, 2, &cameraTaskHandle); 

        xTaskCreate(led_task, "led_task", 1024 * 10, NULL, 3, NULL); 

        xTaskCreate(tcpReceive_task, "tcpReceive_task", 1024 * 50, NULL, 3, NULL); 

        ESP_LOGI("TCP", "MEM = %ld",xPortGetFreeHeapSize()  );
        
        workFlag = 1;
    }

}