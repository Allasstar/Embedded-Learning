#include "oled.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
extern const uint8_t font5x7[];

#define OLED_ADDR (0x3C << 1)

static uint8_t frame[OLED_WIDTH * OLED_HEIGHT / 8];
static volatile uint8_t busy;

static void Commands(const uint8_t *commands, uint16_t count)
{
    HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, (uint8_t *)commands, count, 100);
}

void Oled_Init(void)
{
    // clang-format off
    static const uint8_t init[] =
    {
        0xAE,       // display off
        0xD5, 0x80, // clock divide
        0xA8, 0x3F, // multiplex: 64 rows
        0xD3, 0x00, // display offset 0
        0x40,       // start line 0
        0x8D, 0x14, // charge pump on (makes the high panel voltage from 3.3 V)
        0x20, 0x00, // horizontal addressing mode
        0xA1,       // mirror columns
        0xC8,       // mirror rows
        0xDA, 0x12, // COM pins config for 128x64
        0x81, 0xCF, // contrast
        0xD9, 0xF1, // pre-charge
        0xDB, 0x40, // VCOMH level
        0xA4,       // show RAM contents
        0xA6,       // normal (not inverted)
        0xAF,       // display on
    };
    // clang-format on

    // The panel needs time after power-up before it accepts commands.
    HAL_Delay(100);
    Commands(init, sizeof(init));
    Oled_Clear();
    Oled_Update();
}

void Oled_Clear(void)
{
    memset(frame, 0, sizeof(frame));
}

void Oled_Update(void)
{
    if (busy)
        return;

    static const uint8_t window[] = {0x21, 0, OLED_WIDTH - 1, 0x22, 0, 7};
    Commands(window, sizeof(window));

    // Set before starting: the completion interrupt could fire before the next line runs.
    busy = 1;
    // A failed start fires no callback, so nothing else would ever clear the flag.
    if (HAL_I2C_Mem_Write_DMA(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, frame, sizeof(frame)) != HAL_OK)
        busy = 0;
}

uint8_t Oled_IsBusy(void)
{
    return busy;
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
        busy = 0;
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
        busy = 0;
}

void Oled_DrawPixel(int32_t x, int32_t y, uint8_t on)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
        return;

    uint8_t *cell = &frame[x + (y / 8) * OLED_WIDTH];
    uint8_t mask = 1 << (y % 8);
    if (on)
        *cell |= mask;
    else
        *cell &= ~mask;
}

void Oled_FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t on)
{
    for (int32_t i = x; i < x + w; i++)
        for (int32_t j = y; j < y + h; j++)
            Oled_DrawPixel(i, j, on);
}

void Oled_DrawChar(int32_t x, int32_t y, char c)
{
    const uint8_t *glyph = &font5x7[(uint8_t)c * 5];
    for (int32_t column = 0; column < 5; column++)
    {
        uint8_t bits = glyph[column];
        for (int32_t row = 0; row < 8; row++)
            Oled_DrawPixel(x + column, y + row, bits & (1 << row));
    }
}

void Oled_DrawString(int32_t x, int32_t y, const char *text)
{
    while (*text)
    {
        Oled_DrawChar(x, y, *text++);
        x += 6;
    }
}
