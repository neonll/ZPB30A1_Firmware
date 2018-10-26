#include "adc.h"
#include "stm8s_adc1.h"

void adc_init()
{
    ADC1->CR2 |= ADC1_ALIGN_RIGHT;
    ADC1->CSR |= ADC1_CHANNEL_1;
    ADC1->CR1 |= ADC1_CR1_ADON;
}
