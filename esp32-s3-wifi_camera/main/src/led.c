#include "led.h"
#include "driver/ledc.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (FLASH_CTR_IO) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

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
    ledc_init();
}

void flashLedOn(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (float)deviceAttributeInfo.ledFlashBrightness/100.0 * 8191.0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void flashLedOff(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL,0));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
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


