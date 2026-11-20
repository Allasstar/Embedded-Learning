#include "app.h"
#include "console.h"
#include "log.h"
#include "main.h"
#include "morse.h"
#include "timer.h"

// Keeps blinking during Morse send/receive: visible proof the main loop never blocks.
static Timer heartbeatTimer = {.periodMs = 1000, .activeMs = 500};

void App_Start(void)
{
    LOG("App started\n");
    Console_Init();
    Console_Printf("Morse station ready. Type 'help'.\r\n");
}

void App_Update(void)
{
    Console_Update();
    Morse_Update();

    HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin,
                      Timer_IsActive(&heartbeatTimer) ? GPIO_PIN_SET : GPIO_PIN_RESET);
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
