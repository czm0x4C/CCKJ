#ifndef __UART_H__
#define __UART_H__

enum frameAddress
{
                PC = 0xA5,
                MCU = 0xA6
};
enum frameCmd{
                CDM_WIFI_NAME = 0,/* 帧携带的数据为WIFI的名称 */
                CMD_WIFI_PASSWORD,
                CMD_SERVER_IP,
                CMD_SERVER_PORT,
                CMD_PICTURE_SIZE,
                CMD_PICTURE_QUALITY,
                CMD_SET_PARA_END,
                CMD_LOG_MESSAGE
                    };
void uartInit(void);
void uart_task(void *arg);
unsigned char espSendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data);

#endif /* __CAMERA_H__ */




