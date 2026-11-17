#include "console.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Power of two so the wrap can be a mask instead of a modulo.
#define RX_SIZE 128
#define RX_MASK (RX_SIZE - 1)

extern UART_HandleTypeDef huart2;

static uint8_t rxByte;
static volatile uint8_t rxBuffer[RX_SIZE];
// Single writer per index (head: ISR, tail: main loop), so no locking is needed.
static volatile uint32_t rxHead;
static volatile uint32_t rxTail;

static char line[64];
static uint32_t lineLength;

static void HandleLine(char *text);

void Console_Init(void)
{
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}

void Console_Printf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // vsnprintf returns the untruncated length, which can exceed the buffer.
    if (length > (int)sizeof(buffer) - 1)
        length = sizeof(buffer) - 1;
    if (length > 0)
        HAL_UART_Transmit(&huart2, (uint8_t *)buffer, length, 100);
}

void Console_OnRxByte(void)
{
    uint32_t next = (rxHead + 1) & RX_MASK;
    // Drop the byte when full rather than overwrite data the main loop hasn't read.
    if (next != rxTail)
    {
        rxBuffer[rxHead] = rxByte;
        rxHead = next;
    }
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}

void Console_OnError(void)
{
    // HAL stops receiving on overrun; without re-arming the console goes deaf forever.
    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
}

void Console_Update(void)
{
    while (rxTail != rxHead)
    {
        char c = rxBuffer[rxTail];
        rxTail = (rxTail + 1) & RX_MASK;

        // Terminals end lines with \r, \n or both; skipping empty lines handles all three.
        if (c == '\r' || c == '\n')
        {
            if (lineLength > 0)
            {
                line[lineLength] = '\0';
                HandleLine(line);
                lineLength = 0;
            }
        }
        else if (lineLength < sizeof(line) - 1)
        {
            line[lineLength++] = c;
        }
    }
}

static void HandleLine(char *text)
{
    char *command = strtok(text, " ");
    char *argument = strtok(NULL, " ");

    // A line of only spaces leaves strtok with no token.
    if (!command)
        return;

    if (strcmp(command, "help") == 0)
    {
        Console_Printf("help, status, led on|off\r\n");
    }
    else if (strcmp(command, "status") == 0)
    {
        Console_Printf("uptime %lu ms\r\n", HAL_GetTick());
    }
    else if (strcmp(command, "led") == 0 && argument && strcmp(argument, "on") == 0)
    {
        HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_SET);
        Console_Printf("ok\r\n");
    }
    else if (strcmp(command, "led") == 0 && argument && strcmp(argument, "off") == 0)
    {
        HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, GPIO_PIN_RESET);
        Console_Printf("ok\r\n");
    }
    else
    {
        Console_Printf("unknown: %s\r\n", command);
    }
}
