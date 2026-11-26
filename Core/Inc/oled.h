#pragma once
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

void Oled_Init(void);
void Oled_Clear(void);
void Oled_Update(void);
uint8_t Oled_IsBusy(void);
void Oled_DrawPixel(int32_t x, int32_t y, uint8_t on);
void Oled_FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t on);
void Oled_DrawChar(int32_t x, int32_t y, char c);
void Oled_DrawString(int32_t x, int32_t y, const char *text);
