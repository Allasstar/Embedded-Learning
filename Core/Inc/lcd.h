#pragma once
#include <stdint.h>

void Lcd_Init(void);
void Lcd_Clear(void);
void Lcd_SetCursor(uint8_t row, uint8_t column);
void Lcd_Print(const char *text);
void Lcd_Printf(uint8_t row, const char *format, ...);
