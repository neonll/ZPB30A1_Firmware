#include "adc.h"
#include "config.h"

void adc_init()
{
    ADC1->CR2 |= ADC1_ALIGN_RIGHT;
    ADC1->CSR |= ADC1_CHANNEL_1;
    ADC1->CR1 |= ADC1_CR1_ADON;
}

uint16_t analogRead(ADC1_Channel_TypeDef ch)
{
	uint8_t adcH, adcL;
	ADC1->CSR &= (uint8_t)(~ADC1_CSR_CH);
	ADC1->CSR |= (uint8_t)(ch);

	ADC1->CR1 |= ADC1_CR1_ADON;
	while (!(ADC1->CSR & ADC1_IT_EOC));
	adcL = ADC1->DRL;
	adcH = ADC1->DRH;
	ADC1->CSR &= ~ADC1_IT_EOC;
	return (adcL | ((uint16_t)adcH << 8));
}

uint16_t analogRead12(ADC1_Channel_TypeDef ch)
{
	uint16_t val = 0;
	uint8_t i;
	for (i = 0; i < 4 * ADC_OVERSAMPLING; i++) {
		val += analogRead(ch);
	}
	return val / ADC_OVERSAMPLING;
}
