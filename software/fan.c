#include "fan.h"
#include "load.h"
#include "stm8s_tim3.h"
volatile bool calc_fan = 0;
uint16_t temperature = 0;

void fan_init()
{
	//TIM3->ARRH   = 0x10;
	//TIM3->ARRL   = 0x00;
	TIM3->CCMR2  = TIM3_OCMODE_PWM1 | TIM3_CCMR_OCxPE;
	TIM3->CCER1  = TIM3_CCER1_CC2E;
	TIM3->PSCR   = TIM3_PRESCALER_1; // Prescaler of 1 gives 16 MHz / 2^16 = 244 Hz
	TIM3->CR1   |= TIM3_CR1_CEN | TIM3_CR1_ARPE;
	TIM3->CCR2H  = 0;
	TIM3->CCR2L  = 0;
}


void setFan()
{
	if (temperature > 850) { // Over temperature protection
		TIM3->CCR2H  = 0xFF;
		TIM3->CCR2L  = 0xFF;
		error = ERROR_OTP;
	}
	if (temperature > 400) {
		TIM3->CCR2H  = (temperature * 54) >> 8;
		TIM3->CCR2L  = (uint8_t)(temperature * 54);
	} else if (running) { // Set minimum pwm of 1/3 if switched on
		TIM3->CCR2H  = 0x55;
		TIM3->CCR2L  = 0x55;
	} else {
		TIM3->CCR2H  = 0;
		TIM3->CCR2L  = 0;
	}
}
void getTemp(void); //TODO
void tempFan()
{
	if (calc_fan) {
		getTemp();
		setFan();
		calc_fan = 0;
	}
}
