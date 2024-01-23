#ifndef _OV5640_H
#define _OV5640_H

#include "stm32H7xx_hal.h"

#define OV5640_DEVICE_ADDRESS 0x78

#define OV5640_PWDN         HAL_GPIO_WritePin(GPIOI,GPIO_PIN_11,GPIO_PIN_SET)
#define OV5640_PWUP         HAL_GPIO_WritePin(GPIOI,GPIO_PIN_11,GPIO_PIN_RESET)
#define OV5640_RST  	      HAL_GPIO_WritePin(GPIOI,GPIO_PIN_10,GPIO_PIN_RESET)
#define OV5640_RUN  	      HAL_GPIO_WritePin(GPIOI,GPIO_PIN_10,GPIO_PIN_SET)

uint8_t SCCB_WR_Reg(uint16_t reg,uint8_t data);
uint8_t SCCB_RD_Reg(uint16_t reg);

void SCCB_Rst(void);

//OV5640相关寄存器定义
#define OV5640_CHIPIDH          0X300A  	//OV5640 CHIP ID high byte
#define OV5640_CHIPIDL          0X300B  	//OV5640 CHIP ID low byte

#define OV5640_INIT_OK 		(0x00)
#define OV5640_INIT_FAIL	(0xFF)

/***********************************/

uint8_t OV5640_Init(void);
void OV5640_RGB565_Mode(void);
void OV5640_Exposure(uint8_t exposure);
void OV5640_Light_Mode(uint8_t mode);
void OV5640_Color_Saturation(uint8_t sat);
void OV5640_Brightness(uint8_t bright);
void OV5640_Contrast(uint8_t contrast);
void OV5640_Sharpness(uint8_t sharp);
void OV5640_Special_Effects(uint8_t eft);
uint8_t OV5640_OutSize_Set(uint16_t offx,uint16_t offy,uint16_t width,uint16_t height);
uint8_t OV5640_ImageWin_Set(uint16_t offx,uint16_t offy,uint16_t width,uint16_t height);
uint8_t OV5640_Focus_Init(void);
uint8_t OV5640_Focus_Single(void);
uint8_t OV5640_Focus_Constant(void);
void ov5640_speed_ctrl(void);

#endif

