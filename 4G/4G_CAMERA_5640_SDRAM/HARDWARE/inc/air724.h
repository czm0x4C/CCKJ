#ifndef __AIR724_H
#define __AIR724_H															    

#include "main.h"

#define AIR724_INIT_OK  	(0x00)
#define AIR724_INIT_FAIL 	(0xFF)
#define AIR724_ACK_OK 		(0x00)
#define AIR724_ACK_FAIL 	(0xFF)

typedef struct
{
	uint8_t receiveDataBuffer[1024];		/* 存储接收的ESP发送AT的数据 */
	uint8_t state;											/* 状态 */
	uint8_t getCmdFlag;									/* 开始解析命令 */							
	uint32_t bufferLen;									/* 记录接收的数据长度 */
	uint32_t uartMode;									/* uart工作模式 */		
}AIR724_TypeDef;
 				
extern AIR724_TypeDef air724Struct;

uint8_t air724Init(void);
#endif





