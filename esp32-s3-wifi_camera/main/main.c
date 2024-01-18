#include "wifiConnect.h"
#include "httpServer.h"
#include "camera.h"
#include "uart.h"

void app_main(void)
{
    key_Init();
    LED_Init();/* led初始化 */
    flashLed_Init();
    moto_Init();

    usbSerialPort_Init();/* VCP初始化 */

    // vTaskDelay(5000 / portTICK_PERIOD_MS);/* 延迟5S */

    // unsigned char connectState[10];/*  */
    // char isConnectedFlag = 0;
    // ESP_LOGI("main","readWifiInfo");
    // readWifiInfo(wifiName,wifiPassword,connectState); /* 读取要连接的wifi信息 */
    
    // memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    // sprintf(logMessageBuffer,"ESP:wifiName = %s,wifiPassword = %s",wifiName,wifiPassword);
    // espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);


    // if(strcmp((char *)connectState,"fail") == 0)      /* 保存的wifi信息无效 */
    // {
    //     espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:读取的WIFI信息无效");
    // }
    // else if(strcmp((char *)connectState,"ok") == 0)   /* 保存的信息有效 */
    // {
    //     ESP_LOGI("main","start wifi sta");

    //     memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    //     sprintf(logMessageBuffer,"开始连接WIFI");
    //     espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);

    //     wifi_init_sta((char *)wifiName,(char *)wifiPassword,&isConnectedFlag);  /* 连接指定的wifi */
        
    //     ESP_LOGI("wifiConnect","state = %d",isConnectedFlag);                   /* 判断连接是否成功 */
    //     if(isConnectedFlag == 0)                                                
    //     {
    //         espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:连接WIFI失败");
    //         writeWifiInfo((unsigned char *)"111",(unsigned char *)"111",(unsigned char *)"111",1111,(unsigned char *)"fail");   /* 连接失败处理 */
    //         esp_restart();/* 重启设备 */
    //     }
    //     else if(isConnectedFlag == 1)/* 连接成功处理 */
    //     {
    //         memset(logMessageBuffer,0,sizeof(logMessageBuffer));
    //         sprintf(logMessageBuffer,"WIFI连接成功");
    //         espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)logMessageBuffer);
            
    //         cameraSetConfig();    /* 读取保存的设备信息 */
    //         sntp_Init();                                                       
    //         appInit();
    //     }   
    // }  
}



