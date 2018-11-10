#ifndef _ADC_H_
#define _ADC_H_
#include <stdint.h>
#include "inc/stm8s_adc1.h"

void adc_init();
void adc_timer();
extern uint16_t temperature;
extern uint16_t v_12V;
extern uint16_t v_load;
extern uint16_t v_sense;

#endif
