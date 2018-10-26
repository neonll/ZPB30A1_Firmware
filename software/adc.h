#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>
#include "inc/stm8s_adc1.h"

void adc_init();
uint16_t analogRead(ADC1_Channel_TypeDef ch);
uint16_t analogRead12(ADC1_Channel_TypeDef ch);
#endif
