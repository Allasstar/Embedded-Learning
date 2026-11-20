#include "morse.h"
#include "console.h"
#include "main.h"
#include <ctype.h>
#include <string.h>

#define UNIT_MS 300
#define BOUNCE_MS 20

static const char *const morseCodes[26] = {
    ".-",
    "-...",
    "-.-.",
    "-..",
    ".",
    "..-.",
    "--.",
    "....",
    "..",
    ".---",
    "-.-",
    ".-..",
    "--",
    "-.",
    "---",
    ".--.",
    "--.-",
    ".-.",
    "...",
    "-",
    "..-",
    "...-",
    ".--",
    "-..-",
    "-.--",
    "--..",
};

static char message[64];
static uint32_t messageIndex;
static const char *code;
static uint32_t codeIndex;
static bool sending;
static bool ledOn;
static uint32_t stepStartTick;
static uint32_t stepDurationMs;

static bool keyDown;
static uint32_t pressTick;
static uint32_t releaseTick;
static char tapped[8];
static uint32_t tappedLength;
static bool wordPending;

static void UpdateSender(void);
static void UpdateDecoder(void);
static void SetLed(bool on);
static char Decode(const char *pattern);

void Morse_Send(const char *text)
{
    strncpy(message, text, sizeof(message) - 1);
    message[sizeof(message) - 1] = '\0';
    messageIndex = 0;
    code = NULL;
    SetLed(false);
    stepStartTick = HAL_GetTick();
    stepDurationMs = 0;
    sending = true;
}

bool Morse_IsSending(void)
{
    return sending;
}

void Morse_Update(void)
{
    UpdateSender();
    UpdateDecoder();
}

static void UpdateSender(void)
{
    uint32_t now = HAL_GetTick();
    if (!sending || now - stepStartTick < stepDurationMs)
        return;

    stepStartTick = now;

    if (ledOn)
    {
        SetLed(false);
        if (code[codeIndex] != '\0')
        {
            stepDurationMs = UNIT_MS;
            return;
        }
        code = NULL;
        messageIndex++;
        stepDurationMs = 3 * UNIT_MS;
        return;
    }

    if (!code)
    {
        char c = (char)tolower((unsigned char)message[messageIndex]);
        if (c == '\0')
        {
            sending = false;
            return;
        }
        if (c == ' ')
        {
            // The previous letter already waited 3 units; 4 more makes the 7-unit word gap.
            messageIndex++;
            stepDurationMs = 4 * UNIT_MS;
            return;
        }
        if (c < 'a' || c > 'z')
        {
            messageIndex++;
            stepDurationMs = 0;
            return;
        }
        code = morseCodes[c - 'a'];
        codeIndex = 0;
    }

    char element = code[codeIndex++];
    SetLed(true);
    stepDurationMs = element == '.' ? UNIT_MS : 3 * UNIT_MS;
}

static void UpdateDecoder(void)
{
    uint32_t now = HAL_GetTick();
    bool down = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_SET;

    if (down && !keyDown)
    {
        keyDown = true;
        pressTick = now;
        return;
    }

    if (!down && keyDown)
    {
        keyDown = false;
        uint32_t heldMs = now - pressTick;
        // Bounce pulses must not count as dots or reset the gap timer.
        if (heldMs < BOUNCE_MS)
            return;

        releaseTick = now;
        if (tappedLength < sizeof(tapped) - 1)
            tapped[tappedLength++] = heldMs < 2 * UNIT_MS ? '.' : '-';
        return;
    }

    if (keyDown)
        return;

    uint32_t gapMs = now - releaseTick;

    if (tappedLength > 0 && gapMs > 3 * UNIT_MS)
    {
        tapped[tappedLength] = '\0';
        Console_Printf("%c", Decode(tapped));
        tappedLength = 0;
        wordPending = true;
    }

    if (wordPending && gapMs > 7 * UNIT_MS)
    {
        Console_Printf(" ");
        wordPending = false;
    }
}

static void SetLed(bool on)
{
    ledOn = on;
    HAL_GPIO_WritePin(LD6_GPIO_Port, LD6_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static char Decode(const char *pattern)
{
    for (uint32_t i = 0; i < 26; i++)
    {
        if (strcmp(pattern, morseCodes[i]) == 0)
            return (char)('a' + i);
    }
    return '?';
}
