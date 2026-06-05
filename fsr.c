#include "fsr.h"
#include <stdio.h>
#include <string.h>

/*
 * Pin / Channel map:
 *   PA0 = ADC1_CH5  = FSR_UP
 *   PA1 = ADC1_CH6  = FSR_DOWN
 *   PC0 = ADC1_CH1  = FSR_LEFT
 *   PC1 = ADC1_CH2  = FSR_RIGHT
 */

#define FSR_PRESS_OFFSET 300  /* ADC counts above baseline = pressed */

static uint16_t fsr_baseline[4] = {0, 0, 0, 0};

static uint32_t get_channel(FSR_Channel ch)
{
    switch (ch)
    {
        case FSR_UP:    return 5;
        case FSR_DOWN:  return 6;
        case FSR_LEFT:  return 1;
        case FSR_RIGHT: return 2;
        default:        return 5;
    }
}

void FSR_init(void)
{
    /* Enable clocks */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

    /* PA0, PA1 analog, no pull */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (0*2))) | (3U << (0*2));
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (1*2))) | (3U << (1*2));
    GPIOA->PUPDR &= ~(3U << (0*2));
    GPIOA->PUPDR &= ~(3U << (1*2));

    /* PC0, PC1 analog, no pull */
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (0*2))) | (3U << (0*2));
    GPIOC->MODER = (GPIOC->MODER & ~(3U << (1*2))) | (3U << (1*2));
    GPIOC->PUPDR &= ~(3U << (0*2));
    GPIOC->PUPDR &= ~(3U << (1*2));

    /* ADC common clock HCLK/1 */
    ADC123_COMMON->CCR &= ~ADC_CCR_CKMODE;
    ADC123_COMMON->CCR |=  ADC_CCR_CKMODE_0;

    /* Exit deep power down, enable regulator */
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |=  ADC_CR_ADVREGEN;
    for (volatile uint32_t d = 0; d < 1000; d++);

    /* Calibrate single-ended */
    ADC1->CR &= ~ADC_CR_ADCALDIF;
    ADC1->CR |=  ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    /* 12-bit, right-aligned, single conversion */
    ADC1->CFGR &= ~ADC_CFGR_RES;
    ADC1->CFGR &= ~ADC_CFGR_CONT;
    ADC1->CFGR &= ~ADC_CFGR_ALIGN;

    /* Enable ADC */
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY));
}

uint16_t FSR_read(FSR_Channel ch)
{
    uint32_t adc_ch = get_channel(ch);

    ADC1->SQR1 &= ~ADC_SQR1_L;
    ADC1->SQR1 &= ~ADC_SQR1_SQ1;
    ADC1->SQR1 |=  (adc_ch << ADC_SQR1_SQ1_Pos);

    if (adc_ch <= 9)
    {
        ADC1->SMPR1 &= ~(7UL << (adc_ch * 3));
        ADC1->SMPR1 |=  (3UL << (adc_ch * 3));
    }
    else
    {
        uint32_t shift = (adc_ch - 10) * 3;
        ADC1->SMPR2 &= ~(7UL << shift);
        ADC1->SMPR2 |=  (3UL << shift);
    }

    ADC1->ISR |= ADC_ISR_EOC;
    ADC1->CR  |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC));

    return (uint16_t)(ADC1->DR & 0x0FFF);
}

void FSR_Calibrate(void)
{
    /* Average 10 readings at rest for each FSR to set baseline */
    for (int ch = 0; ch < 4; ch++)
    {
        uint32_t sum = 0;
        for (int j = 0; j < 10; j++)
        {
            sum += FSR_read((FSR_Channel)ch);
            HAL_Delay(10);
        }
        fsr_baseline[ch] = (uint16_t)(sum / 10);
    }
}

uint8_t FSR_isPressed(FSR_Channel ch)
{
    return FSR_read(ch) > (fsr_baseline[ch] + FSR_PRESS_OFFSET);
}
