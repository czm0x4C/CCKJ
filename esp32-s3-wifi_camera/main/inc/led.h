#ifndef __LED_H__
#define __LED_H__
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_NET     (1)
#define LED_WORK    (2)


void LED_Init(void);

void netLedOn(void);

void netLedOff(void);

void workLedOn(void);

void workLedOff(void);

void key_Init(void);

void setKeyValue(int value);

int keyValue(gpio_num_t gpio_num);

void flashLed_Init(void);

void flashLedOn(void);

void flashLedOff(void);

#endif /* __CAMERA_H__ */




