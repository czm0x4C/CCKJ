#include "led.h"

void LED_Init(void)
{
    esp_rom_gpio_pad_select_gpio(LED_NET);
    gpio_set_direction(LED_NET, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
    
    esp_rom_gpio_pad_select_gpio(LED_WORK);
    gpio_set_direction(LED_WORK, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
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

void key_Init(void)
{
    esp_rom_gpio_pad_select_gpio(18);
    gpio_set_direction(18, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_pull_mode(18,GPIO_PULLUP_ONLY);
    gpio_set_level(18, 0);
}

void setKeyValue(int value)
{
    gpio_set_level(18, value);
}

int keyValue(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void flashLed_Init(void)
{
    esp_rom_gpio_pad_select_gpio(15);
    gpio_set_direction(15, GPIO_MODE_OUTPUT);//  把这个GPIO作为输出
}

void flashLedOn(void)
{
    gpio_set_level(15, 1);
}

void flashLedOff(void)
{
    gpio_set_level(15, 0);
}


