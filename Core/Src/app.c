#include "app.h"
#include "blue_button.h"
#include "log.h"
#include "main.h"
#include "oled.h"
#include "timer.h"

#define LED_COUNT (sizeof(ledPins) / sizeof(ledPins[0]))

#define BALL_SIZE 4
#define BALL_AREA_TOP 24

static const uint16_t ledPins[] = {LD4_Pin, LD3_Pin, LD5_Pin, LD6_Pin};
static uint8_t ledIndex = 0;
static Timer blinkTimer = {.periodMs = 1000, .activeMs = 500};
static Timer frameTimer = {.periodMs = 33};
static Timer measureTimer = {.periodMs = 1000};

static int32_t ballX = 10;
static int32_t ballY = BALL_AREA_TOP + 5;
static int32_t ballVelocityX = 2;
static int32_t ballVelocityY = 1;

static void WriteLedPin(uint8_t index, GPIO_PinState pinState);
static void MoveBall(void);
static void DrawFrame(void);

void App_Start(void)
{
    LOG("App started\n");
    Oled_Init();
}

void App_Update(void)
{
    if (BlueButton_WasPressed())
    {
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

    if (Timer_Fired(&frameTimer))
    {
        MoveBall();
        DrawFrame();
    }
}

static void WriteLedPin(uint8_t index, GPIO_PinState pinState)
{
    HAL_GPIO_WritePin(GPIOD, ledPins[index], pinState);
}

static void MoveBall(void)
{
    ballX += ballVelocityX;
    ballY += ballVelocityY;

    if (ballX <= 0 || ballX >= OLED_WIDTH - BALL_SIZE)
        ballVelocityX = -ballVelocityX;
    if (ballY <= BALL_AREA_TOP || ballY >= OLED_HEIGHT - BALL_SIZE)
        ballVelocityY = -ballVelocityY;
}

static void DrawFrame(void)
{
    uint32_t now = HAL_GetTick();
    char text[22];

    Oled_Clear();
    snprintf(text, sizeof(text), "Up %lus", now / 1000);
    Oled_DrawString(0, 0, text);
    Oled_FillRect(0, 12, (now % 5000) * OLED_WIDTH / 5000, 8, 1);
    Oled_FillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, 1);

    uint32_t updateStart = HAL_GetTick();
    Oled_Update();
    uint32_t updateMs = HAL_GetTick() - updateStart;

    if (Timer_Fired(&measureTimer))
        LOG("Oled_Update: %lu ms\n", updateMs);
}
