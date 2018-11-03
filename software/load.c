#include "load.h"
#include "config.h"
#include "adc.h"
#include "settings.h"
#include "inc/stm8s_tim1.h"

uint16_t voltage = 0;	// 0,01V
uint16_t set_current = 0;	// 0,001A
volatile uint32_t mAmpere_seconds = 0;	//mAs
volatile uint32_t mWatt_seconds = 0;	//mWs
bool load_active = 0;
error_t error = ERROR_NONE;

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

void load_disable()
{
    load_active = 0;
    GPIOE->ODR &= ~PINE_ENABLE;
}

void load_enable()
{
    load_active = 1;
    //TODO
    // GPIOE->ODR &= ~PINE_ENABLE;
}

void load_timer()
{
    static uint16_t timer = 0;
    timer++;
    if (timer == F_SYSTICK/F_POWER_CALC) {
        timer = 0;
        // watts can be 60000 max.
        uint32_t mWatt = set_current;
        mWatt *= voltage;
        mWatt /= 100;	//voltage is in 0,01V unit
        mWatt_seconds += mWatt / F_POWER_CALC;
        mAmpere_seconds += set_current / F_POWER_CALC;
    }
}

void getVoltage(void)
{
	uint16_t v1, v_ref, v2, v_load;
	v1 = analogRead12(ADC1_CHANNEL_1);
	//v_load = 1.02217839986557 * v1 - 81.5878664441528;
	v_load = 1.012877085 * v1 - 84.025827; // my new calibration with precision measuring
	v2 = analogRead12(ADC1_CHANNEL_2);
	//v_ref = 0.891348658196074 * v2 - 80.4250357289787;
	v_ref = 0.8921068686 * v2 - 83.353412; // my new calibration with precision measuring
	if (v1 > 85) {
		voltage = v_load;
		if (v_ref >= v_load && v_ref < v_load + 100) {
			voltage = v_ref;
		}
	}
}

void calcPWM(void)
{
	uint16_t pwm;
	uint32_t current;
	switch (settings.mode) {
		case MODE_CC:
			current = settings.setpoints[MODE_CC];
			break;
		case MODE_CW: // I = P / U
			current = settings.setpoints[MODE_CW];
			current *= 100; //voltage is in V/100
			current /= voltage;
			break;
		case MODE_CR: // I = U / R
			current = ((uint32_t) voltage * 10000) / settings.setpoints[MODE_CR]; //R in 0,001 Ohm
			break;
	}
	if (current > 10000) current = 10000;
	if (current < 130) current = 130; //load can't go lower then 130mA even with 0 duty cycle
	set_current = current;
	//pwm = I_PWM_FACTOR * current + I_PWM_OFFSET;
	pwm = 2.275235373 * current - 287.0860829; // Output current is linear function of PWM. Regression gained from real measuring.
	TIM1->CCR1H = pwm >> 8;
	TIM1->CCR1L = (uint8_t) pwm;
	// CC
	// set_value[0];
	// CW
	// set_value[1] / voltage;
	// CR
	// voltage / set_value[2]
}
