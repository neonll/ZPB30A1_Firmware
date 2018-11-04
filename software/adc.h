#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>
#include "inc/stm8s_adc1.h"

void adc_init();
void adc_timer();
extern uint16_t adc_values[];
#endif
