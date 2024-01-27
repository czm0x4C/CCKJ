#ifndef __AIR724_H
#define __AIR724_H															    

#include "main.h"

#define AIR724_INIT_OK  	(0x00)
#define AIR724_INIT_FAIL 	(0xFF)
#define AIR724_ACK_OK 		(0x00)
#define AIR724_ACK_FAIL 	(0xFF)

typedef struct
{
	uint8_t receiveDataBuffer[1024];		/* �洢���յ�ESP����AT������ */
	uint8_t state;											/* ״̬ */
	uint8_t getCmdFlag;									/* ��ʼ�������� */							
	uint32_t bufferLen;									/* ��¼���յ����ݳ��� */
	uint32_t uartMode;									/* uart����ģʽ */		
}AIR724_TypeDef;
 				
extern AIR724_TypeDef air724Struct;

uint8_t air724Init(void);
#endif





