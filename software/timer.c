#include "timer.h"
#include "load.h"
#include "ui.h"
#include "config.h"
#include "inc/stm8s_tim2.h"

volatile uint32_t systick = 0;
volatile uint8_t systick_flag = 0;

void systick_init()
{
	#define SYSTICK_PRESCALER 8
	#define SYSTICK_RELOAD (F_CPU / F_SYSTICK / SYSTICK_PRESCALER)
	TIM2->PSCR   = TIM2_PRESCALER_8;
	TIM2->ARRH   = SYSTICK_RELOAD >> 8;
	TIM2->ARRL   = SYSTICK_RELOAD & 0xff;
	TIM2->IER    = TIM2_IER_UIE;
	TIM2->CR1   |= TIM2_CR1_CEN;
}

//TODO: Move calculations to main loop.
void TIM2_UPD_OVF_Handler() __interrupt(13)
{
	TIM2->SR1 &= ~TIM2_SR1_UIF;

	systick++;
	if (systick % (uint32_t)(F_SYSTICK/F_DISPLAY_REDRAW) == 0) {
		redraw = 1;
	}
	if (systick % (uint32_t)(F_SYSTICK/F_POWER_CALC) == 0 && load_active) {
		// watts can be 60000 max.
		uint32_t mWatt = set_current;
		mWatt *= voltage;
		mWatt /= 100;	//voltage is in 0,01V unit
		mWatt_seconds += mWatt;
		mAmpere_seconds += set_current;
	}
}
