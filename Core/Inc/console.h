#pragma once

void Console_Init(void);
void Console_Update(void);
void Console_Printf(const char *format, ...);
void Console_OnRxByte(void);
void Console_OnError(void);
