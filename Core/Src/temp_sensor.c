#include "temp_sensor.h"
#include "main.h"

// Factory calibration in system memory, measured by ST with VDDA = 3.3 V (RM0090 / STM32F407 datasheet)
#define VREFINT_CAL (*(const uint16_t *)0x1FFF7A2A)
#define TS_CAL1 (*(const uint16_t *)0x1FFF7A2C)
#define TS_CAL2 (*(const uint16_t *)0x1FFF7A2E)
#define TS_CAL1_CELSIUS 30
#define TS_CAL2_CELSIUS 110

#define CHANNEL_TEMPERATURE 16
#define CHANNEL_VREFINT 17
#define SAMPLE_COUNT 16

static uint32_t Convert(uint8_t channel)
{
    ADC1->SQR3 = channel;
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC))
    {
    }
    return ADC1->DR;
}

static uint32_t SumSamples(uint8_t channel)
{
    // The internal sensor is noisy (a few LSB); summing 16 samples steadies the shown degree
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLE_COUNT; i++)
    {
        sum += Convert(channel);
    }
    return sum;
}

void TempSensor_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // PCLK2 84 MHz / 4 = 21 MHz, under the 36 MHz ADC maximum
    ADC123_COMMON->CCR = ADC_CCR_ADCPRE_0 | ADC_CCR_TSVREFE;

    // 480 cycles at 21 MHz = 23 us; the datasheet needs at least 10 us of sampling for the temperature sensor and VREFINT
    ADC1->SMPR1 = ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;

    ADC1->CR2 = ADC_CR2_ADON;

    // Covers the sensor start-up (10 us) and ADC stabilisation (3 us)
    HAL_Delay(1);
}

int32_t TempSensor_ReadCelsius(void)
{
    uint32_t temperatureSum = SumSamples(CHANNEL_TEMPERATURE);
    uint32_t vrefintSum = SumSamples(CHANNEL_VREFINT);

    // The Discovery board runs VDDA at 3.0 V, not the 3.3 V of calibration; VREFINT is a fixed 1.21 V, so the
    // ratio VREFINT_CAL / vrefint rescales the reading to what it would be at 3.3 V
    int32_t raw = (int32_t)(temperatureSum * VREFINT_CAL / vrefintSum);

    int32_t calSpan = TS_CAL2 - TS_CAL1;
    int32_t celsiusSpan = TS_CAL2_CELSIUS - TS_CAL1_CELSIUS;
    return TS_CAL1_CELSIUS + ((raw - TS_CAL1) * celsiusSpan + calSpan / 2) / calSpan;
}
