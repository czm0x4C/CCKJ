#include "ov2640.h"

//for not open-drain bus
uint32_t Tickdelay;
void SCCB_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10|GPIO_PIN_11, GPIO_PIN_SET);
	/*Configure GPIO pins : PB10 PB11 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
void SCCB_Start(void)
{
    SCCB_SDA_H;
    SCCB_SCL_H;

    Tickdelay = ticknumber;while(Tickdelay--);
    SCCB_SDA_L;

    Tickdelay = ticknumber;while(Tickdelay--);
    SCCB_SCL_L;
}


void SCCB_Stop(void)
{
    SCCB_SDA_L;

    Tickdelay = ticknumber;while(Tickdelay--);
    SCCB_SCL_H;

    Tickdelay = ticknumber;while(Tickdelay--);
    SCCB_SDA_H;

    Tickdelay = ticknumber;while(Tickdelay--);
}

void SCCB_No_Ack(void)
{
	HAL_Delay(1);
	SCCB_SDA_H;
	SCCB_SCL_H;

	Tickdelay = ticknumber;while(Tickdelay--);
	SCCB_SCL_L;

	Tickdelay = ticknumber;while(Tickdelay--);
	SCCB_SDA_L;

	Tickdelay = ticknumber;while(Tickdelay--);
}

uint8_t SCCB_WR_Byte(uint8_t dat)
{
	uint8_t j,res;
	for(j=0;j<8;j++)
	{
		if(dat&0x80)SCCB_SDA_H;
		else SCCB_SDA_L;
		dat<<=1;

		Tickdelay = ticknumber;while(Tickdelay--);
		SCCB_SCL_H;

		Tickdelay = ticknumber;while(Tickdelay--);
		SCCB_SCL_L;
	}
	SCCB_SDA_IN();

	Tickdelay = ticknumber;while(Tickdelay--);
	SCCB_SCL_H;

	Tickdelay = ticknumber;while(Tickdelay--);
	if(SCCB_READ_SDA)res=1;
	else res=0;
	SCCB_SCL_L;
	SCCB_SDA_OUT();
	return res;
}

uint8_t SCCB_RD_Byte(void)
{
	uint8_t temp=0,j;
	SCCB_SDA_IN();
	for(j=8;j>0;j--)
	{

		Tickdelay = ticknumber;while(Tickdelay--);
	    SCCB_SCL_H;
		temp=temp<<1;
		if(SCCB_READ_SDA)temp++;

		Tickdelay = ticknumber;while(Tickdelay--);
		SCCB_SCL_L;
	}
	SCCB_SDA_OUT();
	return temp;
}

uint8_t SCCB_WR_Reg(uint8_t reg,uint8_t data)
{
	uint8_t res=0;
	SCCB_Start();
	if(SCCB_WR_Byte(SCCB_ID_W))res=1;

	Tickdelay = ticknumber;while(Tickdelay--);
	if(SCCB_WR_Byte(reg))res=1;

	Tickdelay = ticknumber;while(Tickdelay--);
  	if(SCCB_WR_Byte(data))res=1;
  	SCCB_Stop();
  	return	res;
}

uint8_t SCCB_RD_Reg(uint8_t reg)
{
	uint8_t val=0;
	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID_W);

	Tickdelay = ticknumber;while(Tickdelay--);
  	SCCB_WR_Byte(reg);

  	Tickdelay = ticknumber;while(Tickdelay--);
	SCCB_Stop();

	Tickdelay = ticknumber;while(Tickdelay--);

	SCCB_Start();
	SCCB_WR_Byte(SCCB_ID_R);

	Tickdelay = ticknumber;while(Tickdelay--);
  	val=SCCB_RD_Byte();
  	SCCB_No_Ack();
  	SCCB_Stop();
  	return val;
}


void SCCB_SDA_IN(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  GPIO_InitStruct.Pin = GPIO_PIN_8;
	  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	  GPIO_InitStruct.Pull = GPIO_PULLUP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SCCB_SDA_OUT(void)
{
	  GPIO_InitTypeDef GPIO_InitStruct = {0};
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  GPIO_InitStruct.Pin = GPIO_PIN_8;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SCCB_Rst(void)
{
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_11,GPIO_PIN_RESET)	;
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_RESET)	;
	HAL_Delay(5);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_10,GPIO_PIN_SET)	;
	HAL_Delay(5);
}


//--------------OV2640 Functions--------------//
//UXGA(1600*1200)
const uint8_t ov2640_uxga_init_reg_tbl[][2]=
{
	0xff, 0x00,
	0x2c, 0xff,
	0x2e, 0xdf,
	0xff, 0x01,
	0x3c, 0x32,
	//
	0x11, 0x00,
	0x09, 0x02,
	0x04, 0xD8,
	0x13, 0xe5,
	0x14, 0x48,
	0x2c, 0x0c,
	0x33, 0x78,
	0x3a, 0x33,
	0x3b, 0xfB,
	//
	0x3e, 0x00,
	0x43, 0x11,
	0x16, 0x10,
	//
	0x39, 0x92,
	//
	0x35, 0xda,
	0x22, 0x1a,
	0x37, 0xc3,
	0x23, 0x00,
	0x34, 0xc0,
	0x36, 0x1a,
	0x06, 0x88,
	0x07, 0xc0,
	0x0d, 0x87,
	0x0e, 0x41,
	0x4c, 0x00,

	0x48, 0x00,
	0x5B, 0x00,
	0x42, 0x03,
	//
	0x4a, 0x81,
	0x21, 0x99,
	//
	0x24, 0x40,
	0x25, 0x38,
	0x26, 0x82,
	0x5c, 0x00,
	0x63, 0x00,
	0x46, 0x00,
	0x0c, 0x3c,
	//
	0x61, 0x70,
	0x62, 0x80,
	0x7c, 0x05,
	//
	0x20, 0x80,
	0x28, 0x30,
	0x6c, 0x00,
	0x6d, 0x80,
	0x6e, 0x00,
	0x70, 0x02,
	0x71, 0x94,
	0x73, 0xc1,
	0x3d, 0x34,
	0x5a, 0x57,
	//
	0x12, 0x00,//UXGA 1600*1200

	0x17, 0x11,
	0x18, 0x75,
	0x19, 0x01,
	0x1a, 0x97,
	0x32, 0x36,
	0x03, 0x0f,
	0x37, 0x40,
	//
	0x4f, 0xca,
	0x50, 0xa8,
	0x5a, 0x23,
	0x6d, 0x00,
	0x6d, 0x38,
	//
	0xff, 0x00,
	0xe5, 0x7f,
	0xf9, 0xc0,
	0x41, 0x24,
	0xe0, 0x14,
	0x76, 0xff,
	0x33, 0xa0,
	0x42, 0x20,
	0x43, 0x18,
	0x4c, 0x00,
	0x87, 0xd5,
	0x88, 0x3f,
	0xd7, 0x03,
	0xd9, 0x10,
	0xd3, 0x82,
	//
	0xc8, 0x08,
	0xc9, 0x80,
	//
	0x7c, 0x00,
	0x7d, 0x00,
	0x7c, 0x03,
	0x7d, 0x48,
	0x7d, 0x48,
	0x7c, 0x08,
	0x7d, 0x20,
	0x7d, 0x10,
	0x7d, 0x0e,
	//
	0x90, 0x00,
	0x91, 0x0e,
	0x91, 0x1a,
	0x91, 0x31,
	0x91, 0x5a,
	0x91, 0x69,
	0x91, 0x75,
	0x91, 0x7e,
	0x91, 0x88,
	0x91, 0x8f,
	0x91, 0x96,
	0x91, 0xa3,
	0x91, 0xaf,
	0x91, 0xc4,
	0x91, 0xd7,
	0x91, 0xe8,
	0x91, 0x20,
	//
	0x92, 0x00,
	0x93, 0x06,
	0x93, 0xe3,
	0x93, 0x05,
	0x93, 0x05,
	0x93, 0x00,
	0x93, 0x04,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	//
	0x96, 0x00,
	0x97, 0x08,
	0x97, 0x19,
	0x97, 0x02,
	0x97, 0x0c,
	0x97, 0x24,
	0x97, 0x30,
	0x97, 0x28,
	0x97, 0x26,
	0x97, 0x02,
	0x97, 0x98,
	0x97, 0x80,
	0x97, 0x00,
	0x97, 0x00,
	//
	0xc3, 0xef,

	0xa4, 0x00,
	0xa8, 0x00,
	0xc5, 0x11,
	0xc6, 0x51,
	0xbf, 0x80,
	0xc7, 0x10,
	0xb6, 0x66,
	0xb8, 0xA5,
	0xb7, 0x64,
	0xb9, 0x7C,
	0xb3, 0xaf,
	0xb4, 0x97,
	0xb5, 0xFF,
	0xb0, 0xC5,
	0xb1, 0x94,
	0xb2, 0x0f,
	0xc4, 0x5c,
	//
	0xc0, 0xc8,
	0xc1, 0x96,
	0x8c, 0x00,
	0x86, 0x3d,
	0x50, 0x00,
	0x51, 0x90,
	0x52, 0x2c,
	0x53, 0x00,
	0x54, 0x00,
	0x55, 0x88,

	0x5a, 0x90,
	0x5b, 0x2C,
	0x5c, 0x05,

	0xd3, 0x82,
	//
	0xc3, 0xed,
	0x7f, 0x00,

	0xda, 0x09,

	0xe5, 0x1f,
	0xe1, 0x67,
	0xe0, 0x00,
	0xdd, 0x7f,
	0x05, 0x00,
};
//此模式下,帧率可以达到30帧
//SVGA 800*600
const unsigned char ov2640_svga_init_reg_tbl[][2]= 
{    
	0xff, 0x00,
	0x2c, 0xff,
	0x2e, 0xdf,
	0xff, 0x01,
	0x3c, 0x32,
	//
	0x11, 0x00,
	0x09, 0x02,
	0x04, 0xD8,//水平镜像,垂直翻转
	0x13, 0xe5,
	0x14, 0x48,
	0x2c, 0x0c,
	0x33, 0x78,
	0x3a, 0x33,
	0x3b, 0xfB,
	//
	0x3e, 0x00,
	0x43, 0x11,
	0x16, 0x10,
	//
	0x39, 0x92,
	//
	0x35, 0xda,
	0x22, 0x1a,
	0x37, 0xc3,
	0x23, 0x00,
	0x34, 0xc0,
	0x36, 0x1a,
	0x06, 0x88,
	0x07, 0xc0,
	0x0d, 0x87,
	0x0e, 0x41,
	0x4c, 0x00,
	0x48, 0x00,
	0x5B, 0x00,
	0x42, 0x03,
	//
	0x4a, 0x81,
	0x21, 0x99,
	//
	0x24, 0x40,
	0x25, 0x38,
	0x26, 0x82,
	0x5c, 0x00,
	0x63, 0x00,
	0x46, 0x22,
	0x0c, 0x3c,
	//
	0x61, 0x70,
	0x62, 0x80,
	0x7c, 0x05,
	//
	0x20, 0x80,
	0x28, 0x30,
	0x6c, 0x00,
	0x6d, 0x80,
	0x6e, 0x00,
	0x70, 0x02,
	0x71, 0x94,
	0x73, 0xc1,
	
	0x3d, 0x34, 
	0x5a, 0x57,
	//根据分辨率不同而设置
	0x12, 0x40,//SVGA 800*600
	0x17, 0x11,
	0x18, 0x43,
	0x19, 0x00,
	0x1a, 0x4b,
	0x32, 0x09,
	0x37, 0xc0,
	//
	0x4f, 0xca,
	0x50, 0xa8,
	0x5a, 0x23,
	0x6d, 0x00,
	0x3d, 0x38,
	//
	0xff, 0x00,
	0xe5, 0x7f,
	0xf9, 0xc0,
	0x41, 0x24,
	0xe0, 0x14,
	0x76, 0xff,
	0x33, 0xa0,
	0x42, 0x20,
	0x43, 0x18,
	0x4c, 0x00,
	0x87, 0xd5,
	0x88, 0x3f,
	0xd7, 0x03,
	0xd9, 0x10,
	0xd3, 0x82,
	//
	0xc8, 0x08,
	0xc9, 0x80,
	//
	0x7c, 0x00,
	0x7d, 0x00,
	0x7c, 0x03,
	0x7d, 0x48,
	0x7d, 0x48,
	0x7c, 0x08,
	0x7d, 0x20,
	0x7d, 0x10,
	0x7d, 0x0e,
	//
	0x90, 0x00,
	0x91, 0x0e,
	0x91, 0x1a,
	0x91, 0x31,
	0x91, 0x5a,
	0x91, 0x69,
	0x91, 0x75,
	0x91, 0x7e,
	0x91, 0x88,
	0x91, 0x8f,
	0x91, 0x96,
	0x91, 0xa3,
	0x91, 0xaf,
	0x91, 0xc4,
	0x91, 0xd7,
	0x91, 0xe8,
	0x91, 0x20,
	//
	0x92, 0x00,
	0x93, 0x06,
	0x93, 0xe3,
	0x93, 0x05,
	0x93, 0x05,
	0x93, 0x00,
	0x93, 0x04,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	0x93, 0x00,
	//
	0x96, 0x00,
	0x97, 0x08,
	0x97, 0x19,
	0x97, 0x02,
	0x97, 0x0c,
	0x97, 0x24,
	0x97, 0x30,
	0x97, 0x28,
	0x97, 0x26,
	0x97, 0x02,
	0x97, 0x98,
	0x97, 0x80,
	0x97, 0x00,
	0x97, 0x00,
	//
	0xc3, 0xed,
	0xa4, 0x00,
	0xa8, 0x00,
	0xc5, 0x11,
	0xc6, 0x51,
	0xbf, 0x80,
	0xc7, 0x10,
	0xb6, 0x66,
	0xb8, 0xA5,
	0xb7, 0x64,
	0xb9, 0x7C,
	0xb3, 0xaf,
	0xb4, 0x97,
	0xb5, 0xFF,
	0xb0, 0xC5,
	0xb1, 0x94,
	0xb2, 0x0f,
	0xc4, 0x5c,
	//根据分辨率不同而设置
	0xc0, 0x64,
	0xc1, 0x4B,
	0x8c, 0x00,
	0x86, 0x3D,
	0x50, 0x00,
	0x51, 0xC8,
	0x52, 0x96,
	0x53, 0x00,
	0x54, 0x00,
	0x55, 0x00,
	0x5a, 0xC8,
	0x5b, 0x96,
	0x5c, 0x00,
	
	0xd3, 0x02,//auto设置要小心
	//
	0xc3, 0xed,
	0x7f, 0x00,
	
	0xda, 0x09,
	
	0xe5, 0x1f,
	0xe1, 0x67,
	0xe0, 0x00,
	0xdd, 0x7f,
	0x05, 0x00,
};  
void OV2640_UXGA_Init(void)
{
	for(uint32_t i=0; i<sizeof(ov2640_uxga_init_reg_tbl)/2 ; i++)
 {
   SCCB_WR_Reg(ov2640_uxga_init_reg_tbl[i][0],ov2640_uxga_init_reg_tbl[i][1]);
   if(i<10) HAL_Delay(5);
 }

}

const uint8_t ov2640_rgb565_reg_tbl[][2]=
{
		0xFF, 0x00,
		0xDA, 0x09,
		0xD7, 0x03,
		0xDF, 0x02,
		0x33, 0xa0,
		0x3C, 0x00,
		0xe1, 0x67
};

//OV2640 mode: RGB565
void OV2640_RGB565_Mode(void)
{
	uint16_t i=0;

	for(i=0;i<(sizeof(ov2640_rgb565_reg_tbl)/2);i++)
	{
		SCCB_WR_Reg(ov2640_rgb565_reg_tbl[i][0],ov2640_rgb565_reg_tbl[i][1]);
	}
}

//AUTOEXPOSURE LEVEL PARAMETER: 5 levels
const static uint8_t OV2640_AUTOEXPOSURE_LEVEL[5][8]=
{
    {
        0xFF,0x01,
        0x24,0x20,
        0x25,0x18,
        0x26,0x60,
    },
    {
        0xFF,0x01,
        0x24,0x34,
        0x25,0x1c,
        0x26,0x00,
    },
    {
        0xFF,0x01,
        0x24,0x3e,
        0x25,0x38,
        0x26,0x81,
    },
    {
        0xFF,0x01,
        0x24,0x48,
        0x25,0x40,
        0x26,0x81,
    },
    {
        0xFF,0x01,
        0x24,0x58,
        0x25,0x50,
        0x26,0x92,
    },
};
//Auto_Exposure
//level:0~4
void OV2640_Auto_Exposure(uint8_t level)
{
    uint8_t i;
    uint8_t *p=(uint8_t*)OV2640_AUTOEXPOSURE_LEVEL[level];
    for(i=0;i<4;i++)
    {
        SCCB_WR_Reg(p[i*2],p[i*2+1]);
    }
}
//Light_Mode
//0:auto
//1:sunny
//2:cloudy
//3:office
//4:home
void OV2640_Light_Mode(uint8_t mode)
{
    uint8_t regccval=0X5E;//Sunny
    uint8_t regcdval=0X41;
    uint8_t regceval=0X54;
    switch(mode)
    {
        case 0://auto
            SCCB_WR_Reg(0XFF,0X00);
            SCCB_WR_Reg(0XC7,0X10);//AWB ON
            return;
        case 2://cloudy
            regccval=0X65;
            regcdval=0X41;
            regceval=0X4F;
            break;
        case 3://office
            regccval=0X52;
            regcdval=0X41;
            regceval=0X66;
            break;
        case 4://home
            regccval=0X42;
            regcdval=0X3F;
            regceval=0X71;
            break;
    }
    SCCB_WR_Reg(0XFF,0X00);
    SCCB_WR_Reg(0XC7,0X40);    //AWB OFF
    SCCB_WR_Reg(0XCC,regccval);
    SCCB_WR_Reg(0XCD,regcdval);
    SCCB_WR_Reg(0XCE,regceval);
}
//Color_Saturation
//0:-2
//1:-1
//2,0
//3,+1
//4,+2
void OV2640_Color_Saturation(uint8_t sat)
{
    uint8_t reg7dval=((sat+2)<<4)|0X08;
    SCCB_WR_Reg(0XFF,0X00);
    SCCB_WR_Reg(0X7C,0X00);
    SCCB_WR_Reg(0X7D,0X02);
    SCCB_WR_Reg(0X7C,0X03);
    SCCB_WR_Reg(0X7D,reg7dval);
    SCCB_WR_Reg(0X7D,reg7dval);
}
//Brightness
//0:(0X00)-2
//1:(0X10)-1
//2,(0X20) 0
//3,(0X30)+1
//4,(0X40)+2
void OV2640_Brightness(uint8_t bright)
{
  SCCB_WR_Reg(0xff, 0x00);
  SCCB_WR_Reg(0x7c, 0x00);
  SCCB_WR_Reg(0x7d, 0x04);
  SCCB_WR_Reg(0x7c, 0x09);
  SCCB_WR_Reg(0x7d, bright<<4);
  SCCB_WR_Reg(0x7d, 0x00);
}
//Contrast
//0:-2
//1:-1
//2,0
//3,+1
//4,+2
void OV2640_Contrast(uint8_t contrast)
{
    uint8_t reg7d0val=0X20;
    uint8_t reg7d1val=0X20;
      switch(contrast)
    {
        case 0://-2
            reg7d0val=0X18;
            reg7d1val=0X34;
            break;
        case 1://-1
            reg7d0val=0X1C;
            reg7d1val=0X2A;
            break;
        case 3://1
            reg7d0val=0X24;
            reg7d1val=0X16;
            break;
        case 4://2
            reg7d0val=0X28;
            reg7d1val=0X0C;
            break;
    }
    SCCB_WR_Reg(0xff,0x00);
    SCCB_WR_Reg(0x7c,0x00);
    SCCB_WR_Reg(0x7d,0x04);
    SCCB_WR_Reg(0x7c,0x07);
    SCCB_WR_Reg(0x7d,0x20);
    SCCB_WR_Reg(0x7d,reg7d0val);
    SCCB_WR_Reg(0x7d,reg7d1val);
    SCCB_WR_Reg(0x7d,0x06);
}
//Special_Effects
//0:normal
//1,negative
//2,black-white
//3,red
//4,green
//5,blue
//6,classic
void OV2640_Special_Effects(uint8_t eft)
{
    uint8_t reg7d0val=0X00;
    uint8_t reg7d1val=0X80;
    uint8_t reg7d2val=0X80;
    switch(eft)
    {
        case 1://negative
            reg7d0val=0X40;
            break;
        case 2://black-white
            reg7d0val=0X18;
            break;
        case 3://red
            reg7d0val=0X18;
            reg7d1val=0X40;
            reg7d2val=0XC0;
            break;
        case 4://green
            reg7d0val=0X18;
            reg7d1val=0X40;
            reg7d2val=0X40;
            break;
        case 5://blue
            reg7d0val=0X18;
            reg7d1val=0XA0;
            reg7d2val=0X40;
            break;
        case 6://classic
            reg7d0val=0X18;
            reg7d1val=0X40;
            reg7d2val=0XA6;
            break;
    }
    SCCB_WR_Reg(0xff,0x00);
    SCCB_WR_Reg(0x7c,0x00);
    SCCB_WR_Reg(0x7d,reg7d0val);
    SCCB_WR_Reg(0x7c,0x05);
    SCCB_WR_Reg(0x7d,reg7d1val);
    SCCB_WR_Reg(0x7d,reg7d2val);
}
//Color_Bar
//sw:0,close
//   1,open
void OV2640_Color_Bar(uint8_t sw)
{
    uint8_t reg;
    SCCB_WR_Reg(0XFF,0X01);
    reg=SCCB_RD_Reg(0X12);
    reg&=~(1<<1);
    if(sw)reg|=1<<1;
    SCCB_WR_Reg(0X12,reg);
}

void OV2640_Window_Set(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{
    uint16_t endx;
    uint16_t endy;
    uint8_t temp;
    endx=sx+width/2;
     endy=sy+height/2;

    SCCB_WR_Reg(0XFF,0X01);
    temp=SCCB_RD_Reg(0X03);
    temp&=0XF0;
    temp|=((endy&0X03)<<2)|(sy&0X03);
    SCCB_WR_Reg(0X03,temp);
    SCCB_WR_Reg(0X19,sy>>2);
    SCCB_WR_Reg(0X1A,endy>>2);

    temp=SCCB_RD_Reg(0X32);
    temp&=0XC0;
    temp|=((endx&0X07)<<3)|(sx&0X07);
    SCCB_WR_Reg(0X32,temp);
    SCCB_WR_Reg(0X17,sx>>3);
    SCCB_WR_Reg(0X18,endx>>3);
}

uint8_t OV2640_OutSize_Set(uint16_t width,uint16_t height)
{
    uint16_t outh;
    uint16_t outw;
    uint8_t temp;
    if(width%4)return 1;
    if(height%4)return 2;
    outw=width/4;
    outh=height/4;
    SCCB_WR_Reg(0XFF,0X00);
    SCCB_WR_Reg(0XE0,0X04);
    SCCB_WR_Reg(0X5A,outw&0XFF);
    SCCB_WR_Reg(0X5B,outh&0XFF);
    temp=(outw>>8)&0X03;
    temp|=(outh>>6)&0X04;
    SCCB_WR_Reg(0X5C,temp);
    SCCB_WR_Reg(0XE0,0X00);
    return 0;
}

uint8_t OV2640_ImageWin_Set(uint16_t offx,uint16_t offy,uint16_t width,uint16_t height)
{
    uint16_t hsize;
    uint16_t vsize;
    uint8_t temp;
    if(width%4)return 1;
    if(height%4)return 2;
    hsize=width/4;
    vsize=height/4;
    SCCB_WR_Reg(0XFF,0X00);
    SCCB_WR_Reg(0XE0,0X04);
    SCCB_WR_Reg(0X51,hsize&0XFF);
    SCCB_WR_Reg(0X52,vsize&0XFF);
    SCCB_WR_Reg(0X53,offx&0XFF);
    SCCB_WR_Reg(0X54,offy&0XFF);
    temp=(vsize>>1)&0X80;
    temp|=(offy>>4)&0X70;
    temp|=(hsize>>5)&0X08;
    temp|=(offx>>8)&0X07;
    SCCB_WR_Reg(0X55,temp);
    SCCB_WR_Reg(0X57,(hsize>>2)&0X80);
    SCCB_WR_Reg(0XE0,0X00);
    return 0;
}

uint8_t OV2640_ImageSize_Set(uint16_t width,uint16_t height)
{
    uint8_t temp;
    SCCB_WR_Reg(0XFF,0X00);
    SCCB_WR_Reg(0XE0,0X04);
    SCCB_WR_Reg(0XC0,(width)>>3&0XFF);
    SCCB_WR_Reg(0XC1,(height)>>3&0XFF);
    temp=(width&0X07)<<3;
    temp|=height&0X07;
    temp|=(width>>4)&0X80;
    SCCB_WR_Reg(0X8C,temp);
    SCCB_WR_Reg(0XE0,0X00);
    return 0;
}
//OV2640切换为JPEG模式
const unsigned char ov2640_yuv422_reg_tbl[][2]= 
{
	0xFF, 0x00, 
	0xDA, 0x10,
	0xD7, 0x03,
	0xDF, 0x00,
	0x33, 0x80,
	0x3C, 0x40,
	0xe1, 0x77,
	0x00, 0x00,
};
const unsigned char ov2640_jpeg_reg_tbl[][2]=
{
	0xff, 0x01, 
	0xe0, 0x14,
	0xe1, 0x77,
	0xe5, 0x1f,
	0xd7, 0x03,
	0xda, 0x10,
	0xe0, 0x00, 
};
void OV2640_JPEG_Mode(void) 
{
	unsigned short i=0;
	//设置:YUV422格式
	for(i=0;i<(sizeof(ov2640_yuv422_reg_tbl)/2);i++)
	{
		SCCB_WR_Reg(ov2640_yuv422_reg_tbl[i][0],ov2640_yuv422_reg_tbl[i][1]); 
	} 
	//设置:输出JPEG数据
	for(i=0;i<(sizeof(ov2640_jpeg_reg_tbl)/2);i++)
	{
		SCCB_WR_Reg(ov2640_jpeg_reg_tbl[i][0],ov2640_jpeg_reg_tbl[i][1]);  
	}  
}
void OV2640_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOD_CLK_ENABLE();
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_SET);
	/*Configure GPIO pins : PD2 PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	MX_DCMI_Init();		/*初始化DCMI接口*/
}
unsigned char getPicture = 0;
void OV2640_Init(void)
{
//	SCCB_GPIO_Init();/*初始化SCCB的通讯的GPIO*/
//	OV2640_GPIO_Init();/*初始化摄像头使用的GPIO*/
	char OutDebugString[20];
	
	OV2640_PWDN;
	HAL_Delay(100);
	OV2640_PWUP;
	HAL_Delay(100);
	OV2640_RST;
	HAL_Delay(100);
	OV2640_RUN;
	HAL_Delay(100);
	SCCB_WR_Reg(OV2640_DSP_RA_DLMT, 0x01);	//操作sensor寄存器
 	SCCB_WR_Reg(OV2640_SENSOR_COM7, 0x80);	//软复位OV2640
	
	static unsigned short confirmOV2640_MID = 0;
	static unsigned short confirmOV2640_PID = 0;
	confirmOV2640_MID = SCCB_RD_Reg(OV2640_SENSOR_MIDH);
	confirmOV2640_MID = confirmOV2640_MID << 8;
	confirmOV2640_MID = confirmOV2640_MID | SCCB_RD_Reg(OV2640_SENSOR_MIDL);
//	sprintf(OutDebugString,"MID = %X\r\n",confirmOV2640_MID);
//	CDC_Transmit_FS((unsigned char*)OutDebugString,strlen(OutDebugString));
//	memset(OutDebugString,'\0',sizeof(OutDebugString));
	
	confirmOV2640_PID = SCCB_RD_Reg(OV2640_SENSOR_PIDH);
	confirmOV2640_PID = confirmOV2640_PID << 8;
	confirmOV2640_PID = confirmOV2640_PID | SCCB_RD_Reg(OV2640_SENSOR_PIDL);
//	sprintf(OutDebugString,"HID = %X\r\n",confirmOV2640_PID);
//	CDC_Transmit_FS((unsigned char*)OutDebugString,strlen(OutDebugString));
//	memset(OutDebugString,'\0',sizeof(OutDebugString));

	if((confirmOV2640_MID != OV2640_MID) && (confirmOV2640_PID != OV2640_PID))
	{
		sprintf(OutDebugString,"Camera is Error!\r\n");
		while(CDC_Transmit_HS((unsigned char*)OutDebugString,strlen(OutDebugString)) != USBD_OK);
		memset(OutDebugString,'\0',sizeof(OutDebugString));
		sprintf(OutDebugString,"MID = %X,HID = %X",confirmOV2640_MID,confirmOV2640_PID);
		while(CDC_Transmit_HS((unsigned char*)OutDebugString,strlen(OutDebugString)) != USBD_OK);
		memset(OutDebugString,'\0',sizeof(OutDebugString));
		HAL_Delay(1000);
		return ;
	}
	sprintf(OutDebugString,"Camera is OK!\r\n");
//	while(CDC_Transmit_FS((unsigned char*)OutDebugString,strlen(OutDebugString)) != USBD_OK);
	memset(OutDebugString,'\0',sizeof(OutDebugString));
	if(1)
	{
		unsigned int i = 0;
//		for(i=0;i<sizeof(ov2640_uxga_init_reg_tbl)/2;i++)
//		{
//			SCCB_WR_Reg(ov2640_uxga_init_reg_tbl[i][0],ov2640_uxga_init_reg_tbl[i][1]);
//		} 
		for(i=0;i<sizeof(ov2640_svga_init_reg_tbl)/2;i++)
	{
	   	SCCB_WR_Reg(ov2640_svga_init_reg_tbl[i][0],ov2640_svga_init_reg_tbl[i][1]);
 	}
	OV2640_UXGA_Init();
	
		/*设置工作模式*/
		OV2640_JPEG_Mode();
		//OV2640_RGB565_Mode();
		OV2640_OutSize_Set(800,600);
	}
	else
	{
		OV2640_UXGA_Init();

		//pix speed adjustment
		SCCB_WR_Reg(0xff, 0x00);
		SCCB_WR_Reg(0xd3, 0x00);
		SCCB_WR_Reg(0XFF, 0X01);
		SCCB_WR_Reg(0X11, 0x01);

		OV2640_RGB565_Mode();
		OV2640_OutSize_Set(20,120);		
	}
	
	HAL_Delay(1000);
}

