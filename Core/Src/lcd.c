#include "lcd.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c3;

#define LCD_ADDR (0x27 << 1)
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BACKLIGHT 0x08

static void WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble << 4) | rs | LCD_BACKLIGHT;
    uint8_t sequence[2] = {data | LCD_EN, data};
    HAL_I2C_Master_Transmit(&hi2c3, LCD_ADDR, sequence, 2, 10);
}

static void WriteByte(uint8_t value, uint8_t rs)
{
    WriteNibble(value >> 4, rs);
    WriteNibble(value & 0x0F, rs);
}

static void Command(uint8_t command)
{
    WriteByte(command, 0);
}

void Lcd_Init(void)
{
    // HD44780 "initializing by instruction": 0x3 three times forces 8-bit mode from any state, then 0x2 enters 4-bit
    HAL_Delay(50);
    WriteNibble(0x03, 0);
    HAL_Delay(5);
    WriteNibble(0x03, 0);
    HAL_Delay(1);
    WriteNibble(0x03, 0);
    HAL_Delay(1);
    WriteNibble(0x02, 0);
    HAL_Delay(1);

    Command(0x28);
    Command(0x08);
    Lcd_Clear();
    Command(0x06);
    Command(0x0C);
}

void Lcd_Clear(void)
{
    Command(0x01);
    HAL_Delay(2);
}

void Lcd_SetCursor(uint8_t row, uint8_t column)
{
    // Line 2 starts at DDRAM address 0x40, not 16
    Command(0x80 | ((row ? 0x40 : 0x00) + column));
}

void Lcd_Print(const char *text)
{
    while (*text)
    {
        WriteByte((uint8_t)*text++, LCD_RS);
    }
}

void Lcd_Printf(uint8_t row, const char *format, ...)
{
    char buffer[17];
    char padded[17];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Pad to 16 chars to overwrite leftovers instead of Lcd_Clear, which flickers
    snprintf(padded, sizeof(padded), "%-16s", buffer);
    Lcd_SetCursor(row, 0);
    Lcd_Print(padded);
}
