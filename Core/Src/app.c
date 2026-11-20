#include "app.h"
#include "blue_button.h"
#include "log.h"
#include "main.h"
#include "timer.h"

extern I2C_HandleTypeDef hi2c3;

#define LED_COUNT (sizeof(ledPins) / sizeof(ledPins[0]))

static const uint16_t ledPins[] = {LD4_Pin, LD3_Pin, LD5_Pin, LD6_Pin};
static uint8_t ledIndex = 0;
static Timer blinkTimer = {.periodMs = 1000, .activeMs = 500};

static void WriteLedPin(uint8_t index, GPIO_PinState pinState);

void App_Start(void)
{
    LOG("App started\n");

    for (uint16_t address = 1; address < 128; address++)
    {
        if (HAL_I2C_IsDeviceReady(&hi2c3, address << 1, 2, 5) == HAL_OK)
        {
            LOG("found 0x%02X\n", address);
        }
    }
}

void App_Update(void)
{
    if (BlueButton_WasPressed())
    {
        ledIndex = (ledIndex + 1) % LED_COUNT;
        LOG("ledIndex = %d\n", ledIndex);

        for (uint8_t i = 0; i < LED_COUNT; i++)
        {
            WriteLedPin(i, GPIO_PIN_RESET);
        }
    }

    if (Timer_IsActive(&blinkTimer))
    {
        WriteLedPin(ledIndex, GPIO_PIN_SET);
    }
    else
    {
        WriteLedPin(ledIndex, GPIO_PIN_RESET);
    }
}

static void WriteLedPin(uint8_t index, GPIO_PinState pinState)
{
    HAL_GPIO_WritePin(GPIOD, ledPins[index], pinState);
}
