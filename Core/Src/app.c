#include "app.h"
#include "blue_button.h"
#include "lcd.h"
#include "log.h"
#include "main.h"
#include "timer.h"

#define LED_COUNT (sizeof(ledPins) / sizeof(ledPins[0]))

static const uint16_t ledPins[] = {LD4_Pin, LD3_Pin, LD5_Pin, LD6_Pin};
static uint8_t ledIndex = 0;
static Timer blinkTimer = {.periodMs = 1000, .activeMs = 500};

// LCD crystals are slow; redrawing faster than ~5 Hz only smears and wastes ~20 ms of blocking I2C each time
static Timer dashboardTimer = {.periodMs = 200};
static uint32_t pressCount = 0;
static uint32_t slowestDashboardMs = 0;

static void WriteLedPin(uint8_t index, GPIO_PinState pinState);
static void Dashboard_Update(void);

void App_Start(void)
{
    LOG("App started\n");

    Lcd_Init();
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

    if (Timer_Fired(&dashboardTimer))
    {
        uint32_t startMs = HAL_GetTick();
        Dashboard_Update();
        uint32_t elapsedMs = HAL_GetTick() - startMs;

        // Logging only new maximums keeps the measurement visible without flooding the log 5 times a second
        if (elapsedMs > slowestDashboardMs)
        {
            slowestDashboardMs = elapsedMs;
            LOG("Dashboard_Update took %lu ms\n", elapsedMs);
        }
    }
}

static void Dashboard_Update(void)
{
    Lcd_Printf(0, "Up %lus", HAL_GetTick() / 1000);
    Lcd_Printf(1, "Presses %lu", pressCount);
}

static void WriteLedPin(uint8_t index, GPIO_PinState pinState)
{
    HAL_GPIO_WritePin(GPIOD, ledPins[index], pinState);
}
