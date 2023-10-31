#include "led.h"

void LED_Init(void)
{
    esp_rom_gpio_pad_select_gpio(LED_NET);
    gpio_set_direction(LED_NET, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
    
    esp_rom_gpio_pad_select_gpio(LED_WORK);
    gpio_set_direction(LED_WORK, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出

    netLedOff();
    workLedOff();
}

void netLedOn(void)
{
    gpio_set_level(LED_NET, 0);
}

void netLedOff(void)
{
    gpio_set_level(LED_NET, 1);
}

void workLedOn(void)
{
    gpio_set_level(LED_WORK, 0);
}

void workLedOff(void)
{
    gpio_set_level(LED_WORK, 1);
}

void workLedToggle(void)
{
    static char pinValue = 0;
    if(pinValue == 1)
    {
        workLedOn();
        pinValue = 0;
    }
    else
    {
        workLedOff();
        pinValue = 1;
    }
}

void key_Init(void)
{
    esp_rom_gpio_pad_select_gpio(KEY_IO);
    gpio_set_direction(KEY_IO, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_pull_mode(KEY_IO,GPIO_PULLUP_ONLY);
    gpio_set_level(KEY_IO, 1);
}

void setKeyValue(int value)
{
    gpio_set_level(KEY_IO, value);
}

int keyValue(gpio_num_t gpio_num)
{
    return gpio_get_level(KEY_IO);
}

void flashLed_Init(void)
{
    esp_rom_gpio_pad_select_gpio(FLASH_CTR_IO);
    gpio_set_direction(FLASH_CTR_IO, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
    flashLedOff();
}

void flashLedOn(void)
{
    gpio_set_level(FLASH_CTR_IO, 1);
}

void flashLedOff(void)
{
    gpio_set_level(FLASH_CTR_IO, 0);
}

void moto_Init()
{
    esp_rom_gpio_pad_select_gpio(ELECTRIC_RELAY_IO);
    gpio_set_direction(ELECTRIC_RELAY_IO, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
    motoOff();
}

void motoOn()
{
    gpio_set_level(ELECTRIC_RELAY_IO, 1);
}

void motoOff()
{
    gpio_set_level(ELECTRIC_RELAY_IO, 0);
}