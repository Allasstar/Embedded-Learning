#pragma once
#include <stdint.h>

#define LCD_COLUMNS 16

void Lcd_Init(void);
void Lcd_Clear(void);
void Lcd_CreateChar(uint8_t index, const uint8_t pattern[8]);
void Lcd_Printf(uint8_t row, const char *format, ...);
void Lcd_Update(void);
