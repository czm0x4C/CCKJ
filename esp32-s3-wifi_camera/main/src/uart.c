#include "uart.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "wifiConnect.h"
#include "camera.h"

#define UART_PIN_TXD (43)
#define UART_PIN_RXD (44)

// #define UART_PIN_TXD (16)
// #define UART_PIN_RXD (17)
#define UART_PIN_RTS (-1)
#define UART_PIN_CTS (-1)

#define UART_PORT_NUM      (2)
#define UART_BAUD_RATE     (115200)

/******************************************************************************环形缓冲区**************************************************************************************/
#define  RINGBUFF_LEN          (1024)     //定义最大接收字节数 4k
#define  RINGBUFF_OK           1
#define  RINGBUFF_ERR          0


typedef struct
{
    unsigned short Head;
    unsigned short Tail;
    unsigned short Lenght;
    unsigned char  RingBufferData[RINGBUFF_LEN];
}__attribute__((packed)) _RingBuffer;

/*初始化环形缓冲区*/
void RingBuffer_Init(_RingBuffer *ringbuffer)
{
	ringbuffer->Head   = 0;
	ringbuffer->Tail   = 0;
	ringbuffer->Lenght = 0;
	for(int i=0;i<RINGBUFF_LEN;i++)
	{
		ringbuffer->RingBufferData[i] = '\0';
	}
}
/*缓冲区写入一个字节*/
unsigned char WriteOneByte_RingBuffer(_RingBuffer *ringbuffer, unsigned char OneByteData)
{
    if(ringbuffer->Lenght >= RINGBUFF_LEN)
    {
        return RINGBUFF_ERR;/*缓冲区数据满了*/
    }
    ringbuffer->RingBufferData[ringbuffer->Tail] = OneByteData;/*缓冲区写入一个数据*/

    ringbuffer->Tail = (ringbuffer->Tail +1) % RINGBUFF_LEN;

    ringbuffer->Lenght += 1;

    return RINGBUFF_OK;
}
/*缓冲区写入多个字节*/
unsigned char WriteBytes(_RingBuffer *ringbuffer, unsigned char *Bytes,unsigned short DataLen)
{

    if(ringbuffer->Lenght >= RINGBUFF_LEN)
    {
        return RINGBUFF_ERR;/*缓冲区数据满了*/
    }

    for (int i = 0; i < DataLen; i++)
    {
        WriteOneByte_RingBuffer(ringbuffer,Bytes[i]);
    }
    return RINGBUFF_OK;
}

unsigned char ReadOneByte_RingBuffer(_RingBuffer *ringbuffer,unsigned char *OneByteData)
{
    if(ringbuffer->Lenght == 0)
    {
        return RINGBUFF_ERR;/*缓冲区为空*/
    }
    *OneByteData = ringbuffer->RingBufferData[ringbuffer->Head];/*缓冲区读取一个字节*/

    ringbuffer->Head = (ringbuffer->Head + 1) % RINGBUFF_LEN;

    ringbuffer->Lenght -= 1;

    return RINGBUFF_OK;
}

unsigned char ReadBytes(_RingBuffer *ringbuffer,unsigned char *ReadBuffer, unsigned short Len)
{
    if(Len > ringbuffer->Lenght || ringbuffer->Lenght == 0)
    {
        return 0;           /*读取的数据大于缓冲区的数据,或者缓冲区无数据，返回空数据*/
    }

    for(int i=0;i<Len;i++)
    {
        ReadOneByte_RingBuffer(ringbuffer,&ReadBuffer[i]);
    }
		return 1;
}

unsigned short GetDataLength(unsigned char *DataBuffer)
{
	unsigned short DataLen = 0;
	while(DataBuffer[DataLen] != '\0')
	{
		DataLen++;
	}
	return DataLen;
}
/*帧校验*/
unsigned char FrameDataCheck(unsigned char* OneFrameData)
{
	unsigned char sumcheck = 0;
	unsigned char addcheck = 0;
	unsigned char Len = OneFrameData[3];
	
	for(unsigned char i=0; i < Len + 4 ; i++)
	{
			sumcheck += OneFrameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
			addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
	}
	
	if(sumcheck == OneFrameData[Len + 4] && addcheck == OneFrameData[Len + 5])
	{
			return 1; //校验通过
	}
	else
	{
			return 0; //校验失败
	}
}
/*帧解析函数*/
_RingBuffer UART_RxRingBuffer;
void FrameAnalysis(_RingBuffer *ringbuffer)
{
		unsigned char *RxFrameData;/*存储解析帧的空间*/
		unsigned short FrameDataLen = UART_RxRingBuffer.Lenght;
		if(xPortGetFreeHeapSize() >= FrameDataLen)
		{
			RxFrameData = (unsigned char *)pvPortMalloc(FrameDataLen);	/*为读取缓冲区中的所有数据申请内存*/
			if(ReadBytes(ringbuffer,RxFrameData,FrameDataLen) == 1)		/*读取缓冲区中的所有数据*/
			{
				/*读取到缓冲区的数据*/
				for(int i=0;i<FrameDataLen;i++)	/*遍历*/
				{
					if(RxFrameData[i] == 0xAA)			/*找帧头*/
					{
						if(FrameDataLen - i <= 3)		/*帧头后面无数据长度*/
						{
							WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
							vPortFree(RxFrameData);  //释放内存
							return ;/*返回空数据*/
						}
						
						unsigned short FrameLen = RxFrameData[i+3];			/*得到帧数据内容长度*/
						
						if((FrameDataLen - i) >= (4 + FrameLen + 2))	/*剩余数据是否还有一帧的长度*/
						{
							if(FrameDataCheck( &RxFrameData[i] ) == 1)		/*校验帧数据*/
							{
								unsigned short OneFrameDataLen = 4 + FrameLen + 2;			/*返回帧总长*/
								if(RxFrameData[i+1] == PC)														/*是上位机发送的数据*/
								{
                                    switch (RxFrameData[i+2])
                                    {
                                        case CDM_WIFI_NAME:
                                        {
                                            memset(wifiName,0,sizeof(wifiName));
                                            memcpy((char *)wifiName, &RxFrameData[i+4],FrameLen);
                                            break;
                                        }
                                        case CMD_WIFI_PASSWORD:
                                        {
                                            memset(wifiPassword,0,sizeof(wifiPassword));
                                            memcpy((char *)wifiPassword, &RxFrameData[i+4],FrameLen);
                                            break;
                                        }
                                        case CMD_SERVER_IP:
                                        {
                                            memset(deviceAttributeInfo.UDP_serverIP,0,sizeof(deviceAttributeInfo.UDP_serverIP));
                                            memcpy((char *)deviceAttributeInfo.UDP_serverIP, &RxFrameData[i+4],FrameLen);
                                            break;
                                        }
                                        case CMD_SERVER_PORT:
                                        {
                                            char temp[10];
                                            memcpy((char *)temp, &RxFrameData[i+4],FrameLen);
                                            deviceAttributeInfo.UDP_serverPort = atoi(temp);
                                            break;
                                        }
                                        case CMD_PICTURE_SIZE:
                                        {
                                            camera_config.frame_size = RxFrameData[i+4] | RxFrameData[i+5] <<8;
                                            // uart_write_bytes(ECHO_UART_PORT_NUM, &RxFrameData[i+4], FrameLen);
                                            break;
                                        }
                                        case CMD_PICTURE_QUALITY:
                                        {
                                            camera_config.jpeg_quality = RxFrameData[i+4] | RxFrameData[i+5] <<8;
                                            // uart_write_bytes(ECHO_UART_PORT_NUM, &RxFrameData[i+4], FrameLen);
                                            break;
                                        }
                                        case CMD_SET_PARA_END:
                                        {
                                            writeWifiInfo(wifiName,wifiPassword,(unsigned char *)deviceAttributeInfo.UDP_serverIP,deviceAttributeInfo.UDP_serverPort,(unsigned char *)"ok");
                                            setCameraPara();
                                            espSendLogMessage(0xAA,MCU,CMD_LOG_MESSAGE,(char*)"ESP:数据写入完成,2s后重启");
                                            vTaskDelay(2000/portTICK_PERIOD_MS);
                                            esp_restart();/* 重启设备 */
                                            break;
                                        }
                                        default:
                                            break;
                                    }
								}
								i += OneFrameDataLen-1;										/*偏移*/
							}
						}
						else
						{
							WriteBytes(ringbuffer,&RxFrameData[i],FrameDataLen - i);/*未处理的数据写回缓冲区*/
							vPortFree(RxFrameData);  //释放内存
							return ;
						}
					}
				}
			}
			vPortFree(RxFrameData);  //释放内存
		}
}

unsigned char espSendLogMessage(unsigned char frameHead, unsigned char frameAddress, unsigned char frameID, char* data)
{
    unsigned char frameDataLen = 0;

    char frameData[260];
    frameDataLen = strlen(data);
    frameData[0] = frameHead;                               /* 帧头 */
    frameData[1] = frameAddress;                            /* 帧地址 */
    frameData[2] = frameID;                                 /* 帧ID */
    frameData[3] = frameDataLen;                            /* 帧数据长度 */

    for(int i=0;i<frameDataLen;i++)
    {
        frameData[4+i] = data[i];
    }

    unsigned char sumcheck = 0;
    unsigned char addcheck = 0;
    for(unsigned char i=0; i < frameData[3] + 4 ; i++)
    {
        sumcheck += frameData[i]; //从帧头开始，对每一字节进行求和，直到DATA区结束
        addcheck += sumcheck; //每一字节的求和操作，进行一次sumcheck的累加
    }
    frameData[frameDataLen + 4] = sumcheck;
    frameData[frameDataLen + 5] = addcheck;
    
    return uart_write_bytes(UART_PORT_NUM,frameData,frameDataLen + 4 + 2);
}

#define BUF_SIZE (1024)

void uartInit(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_PIN_TXD, UART_PIN_RXD, UART_PIN_RTS, UART_PIN_CTS));
}
static const char *TAG = "UART";



void uart_task(void *arg)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) 
    {
        // Read data from the UART
        int len = uart_read_bytes(UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        /* 解析数据 */
        WriteBytes(&UART_RxRingBuffer,data,len);
        FrameAnalysis(&UART_RxRingBuffer);
    }
}