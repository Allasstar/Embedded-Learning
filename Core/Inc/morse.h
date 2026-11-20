#pragma once
#include <stdbool.h>

void Morse_Send(const char *text);
bool Morse_IsSending(void);
void Morse_Update(void);
