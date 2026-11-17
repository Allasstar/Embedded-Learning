#include "app.h"
#include "blue_button.h"
#include "console.h"
#include "log.h"
#include "main.h"
#include "timer.h"

#define LED_COUNT (sizeof(ledPins) / sizeof(ledPins[0]))

// LD6 is left out because the console's "led" command owns it.
static const uint16_t ledPins[] = {LD4_Pin, LD3_Pin, LD5_Pin};
static uint8_t ledIndex = 0;
static Timer blinkTimer = { .periodMs = 1000, .activeMs = 500 };

static void WriteLedPin(uint8_t index, GPIO_PinState pinState);
static void MeasureTransmitCost(void);

void App_Start(void)
{
    LOG("App started\n");
    Console_Init();
    Console_Printf("Hello from STM32\r\n");
    MeasureTransmitCost();
}

void App_Update(void)
{
    Console_Update();

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

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
        Console_OnRxByte();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
        Console_OnError();
}

static void WriteLedPin(uint8_t index, GPIO_PinState pinState)
{
    HAL_GPIO_WritePin(GPIOD, ledPins[index], pinState);
}

static void MeasureTransmitCost(void)
{
    uint32_t start = HAL_GetTick();
    Console_Printf("0123456789012345678901234567890123456789012345678\r\n");
    LOG("50 chars took %lu ms\n", HAL_GetTick() - start);
}
