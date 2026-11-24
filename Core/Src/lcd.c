#include "lcd.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c3;

#define LCD_ADDR (0x27 << 1)
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BACKLIGHT 0x08

#define LCD_ROWS 2
#define LCD_CELLS (LCD_ROWS * LCD_COLUMNS)
#define CURSOR_UNKNOWN 0xFF

static char desired[LCD_CELLS];
static char shown[LCD_CELLS];
static uint8_t nextCell = 0;
static uint8_t cursorCell = CURSOR_UNKNOWN;

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

static void SetCursor(uint8_t row, uint8_t column)
{
    // Line 2 starts at DDRAM address 0x40, not 16
    Command(0x80 | ((row ? 0x40 : 0x00) + column));
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
    memset(desired, ' ', sizeof(desired));
    memset(shown, ' ', sizeof(shown));
    cursorCell = 0;
}

void Lcd_CreateChar(uint8_t index, const uint8_t pattern[8])
{
    Command(0x40 | (index << 3));
    for (uint8_t i = 0; i < 8; i++)
    {
        WriteByte(pattern[i], LCD_RS);
    }
    // The address counter now points into CGRAM, so the next character must set a DDRAM address first
    cursorCell = CURSOR_UNKNOWN;
}

void Lcd_Printf(uint8_t row, const char *format, ...)
{
    char buffer[LCD_COLUMNS + 1];
    char padded[LCD_COLUMNS + 1];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Pad to 16 chars to overwrite leftovers instead of Lcd_Clear, which flickers
    snprintf(padded, sizeof(padded), "%-16s", buffer);
    memcpy(&desired[row * LCD_COLUMNS], padded, LCD_COLUMNS);
}

void Lcd_Update(void)
{
    // At most one character per call keeps the blocking I2C time to ~1 ms instead of ~20 ms for a full redraw
    for (uint8_t i = 0; i < LCD_CELLS; i++)
    {
        uint8_t cell = (nextCell + i) % LCD_CELLS;
        if (desired[cell] == shown[cell])
        {
            continue;
        }

        uint8_t row = cell / LCD_COLUMNS;
        uint8_t column = cell % LCD_COLUMNS;

        // The LCD auto-advances after each character, so a run of changed cells needs only one cursor command
        if (cell != cursorCell)
        {
            SetCursor(row, column);
        }
        WriteByte((uint8_t)desired[cell], LCD_RS);
        shown[cell] = desired[cell];

        // After column 15 the address counter goes to 0x10, which is off-screen, not to the start of line 2
        cursorCell = (column == LCD_COLUMNS - 1) ? CURSOR_UNKNOWN : cell + 1;
        nextCell = (cell + 1) % LCD_CELLS;
        return;
    }
}
