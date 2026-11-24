#include "app.h"
#include "blue_button.h"
#include "lcd.h"
#include "log.h"
#include "main.h"
#include "temp_sensor.h"
#include "timer.h"

#define LED_COUNT (sizeof(ledPins) / sizeof(ledPins[0]))

// Custom character 0 is avoided: a 0 byte would terminate the strings passed to Lcd_Printf
#define BAR_CHAR_FIRST 1
#define BAR_STEPS_PER_CELL 5
#define BAR_STEPS (LCD_COLUMNS * BAR_STEPS_PER_CELL)
#define DEGREE_CHAR 6
#define DEGREE "\x06"
#define SAWTOOTH_PERIOD_MS 10000

static const uint16_t ledPins[] = {LD4_Pin, LD3_Pin, LD5_Pin, LD6_Pin};
static uint8_t ledIndex = 0;
static Timer blinkTimer = {.periodMs = 1000, .activeMs = 500};

// Only changed characters reach the LCD now, so a faster rate costs little and keeps the 80-step bar smooth (one step = 125 ms)
static Timer dashboardTimer = {.periodMs = 50};
// Die temperature changes over seconds, and each read blocks ~1.5 ms for 32 conversions
static Timer temperatureTimer = {.periodMs = 1000};
static uint32_t pressCount = 0;
static int32_t temperatureCelsius = 0;

static const uint8_t degreePattern[8] = {
    0b00110,
    0b01001,
    0b01001,
    0b00110,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
};

static void WriteLedPin(uint8_t index, GPIO_PinState pinState);
static void CreateBarChars(void);
static void Dashboard_Update(void);
static void BuildBar(char *bar, uint32_t steps);

void App_Start(void)
{
    LOG("App started\n");

    Lcd_Init();
    CreateBarChars();
    Lcd_CreateChar(DEGREE_CHAR, degreePattern);

    TempSensor_Init();
    temperatureCelsius = TempSensor_ReadCelsius();
}

void App_Update(void)
{
    if (BlueButton_WasPressed())
    {
        pressCount++;
        ledIndex = (ledIndex + 1) % LED_COUNT;
        LOG("ledIndex = %d\n", ledIndex);

        for (uint8_t i = 0; i < LED_COUNT; i++)
        {
            WriteLedPin(i, GPIO_PIN_RESET);
        }
    }

    if (Timer_IsActive(&blinkTimer))
    {
        WriteLedPin(ledIndex, GPIO_PIN_SET);
    }
    else
    {
        WriteLedPin(ledIndex, GPIO_PIN_RESET);
    }

    if (Timer_Fired(&temperatureTimer))
    {
        temperatureCelsius = TempSensor_ReadCelsius();
    }

    if (Timer_Fired(&dashboardTimer))
    {
        Dashboard_Update();
    }

    Lcd_Update();
}

static void CreateBarChars(void)
{
    uint8_t pattern[8];
    for (uint8_t filled = 1; filled <= BAR_STEPS_PER_CELL; filled++)
    {
        // Bit 4 is the leftmost pixel, so the bar grows left to right
        uint8_t row = (0x1F << (BAR_STEPS_PER_CELL - filled)) & 0x1F;
        for (uint8_t i = 0; i < 8; i++)
        {
            pattern[i] = row;
        }
        Lcd_CreateChar(BAR_CHAR_FIRST + filled - 1, pattern);
    }
}

static void Dashboard_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t phaseMs = now % SAWTOOTH_PERIOD_MS;
    // BAR_STEPS + 1 so the last 125 ms of the period shows a full bar instead of never reaching it
    uint32_t steps = phaseMs * (BAR_STEPS + 1) / SAWTOOTH_PERIOD_MS;

    char bar[LCD_COLUMNS + 1];
    BuildBar(bar, steps);

    Lcd_Printf(0, "%lus P%lu %ld" DEGREE "C", now / 1000, pressCount, temperatureCelsius);
    Lcd_Printf(1, "%s", bar);
}

static void BuildBar(char *bar, uint32_t steps)
{
    uint32_t fullCells = steps / BAR_STEPS_PER_CELL;
    uint32_t remainder = steps % BAR_STEPS_PER_CELL;

    for (uint32_t column = 0; column < LCD_COLUMNS; column++)
    {
        if (column < fullCells)
        {
            bar[column] = BAR_CHAR_FIRST + BAR_STEPS_PER_CELL - 1;
        }
        else if (column == fullCells && remainder > 0)
        {
            bar[column] = BAR_CHAR_FIRST + remainder - 1;
        }
        else
        {
            bar[column] = ' ';
        }
    }
    bar[LCD_COLUMNS] = '\0';
}

static void WriteLedPin(uint8_t index, GPIO_PinState pinState)
{
    HAL_GPIO_WritePin(GPIOD, ledPins[index], pinState);
}
