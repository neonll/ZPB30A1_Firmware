#include "load.h"
#include "config.h"
#include "inc/stm8s_tim1.h"
uint16_t voltage = 0;	// 0,01V
uint16_t set_current = 0;	// 0,001A
volatile uint32_t mAmpere_seconds = 0;	//mAs
volatile uint32_t mWatt_seconds = 0;	//mWs
bool running = 0;


void load_init()
{
    #define PWM_RELOAD (F_CPU / F_PWM)
    // I-SET
	TIM1->ARRH   = PWM_RELOAD >> 8;
	TIM1->ARRL   = PWM_RELOAD & 0xff;
	TIM1->PSCRH  = 0;
	TIM1->PSCRL  = 0;

	TIM1->CCMR1  = TIM1_OCMODE_PWM1 | TIM1_CCMR_OCxPE;
	TIM1->CCER1  = TIM1_CCER1_CC1E;
	TIM1->CCR1H  = 0;
	TIM1->CCR1L  = 0;
	TIM1->CR1   |= TIM1_CR1_CEN | TIM1_CR1_ARPE;
	TIM1->BKR   |= TIM1_BKR_MOE;
}
