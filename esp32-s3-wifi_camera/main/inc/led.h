#ifndef __LED_H__
#define __LED_H__
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_NET     (5)
#define LED_WORK    (4)

#define FLASH_CTR_IO        (42)
#define FLASH_CTR_EXTRA_IO  (17)
#define ELECTRIC_RELAY_IO   (18)
#define KEY_IO              (16)

void LED_Init(void);

void netLedOn(void);

void netLedOff(void);

void workLedOn(void);

void workLedOff(void);

void workLedToggle(void);

void key_Init(void);

void setKeyValue(int value);

int keyValue(gpio_num_t gpio_num);

void flashLed_Init(void);

void flashLedOn(void);

void flashLedOff(void);

void moto_Init();

void motoOn();

void motoOff();

#endif /* __CAMERA_H__ */




