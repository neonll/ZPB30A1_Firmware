#include "timer.h"
#include "load.h"
#include "stm8s_tim2.h"
#include "ui.h"
#include "fan.h"

volatile uint32_t tenmillis          = 0;	// 10 milliseconds
//TODO: Move calculations to main loop.
void TIM2_UPD_OVF_Handler() __interrupt(13)
{
	TIM2->SR1 &= ~TIM2_SR1_UIF;

	tenmillis++;
	if (tenmillis % 50 == 0) {
		redraw = 1;
	}
	if (tenmillis % 100 == 0 && running) {
		// watts can be 60000 max.
		uint32_t mWatt = set_current;
		mWatt *= voltage;
		mWatt /= 100;	//voltage is in 0,01V unit
		mWatt_seconds += mWatt;
		mAmpere_seconds += set_current;
	}
	if (tenmillis % 5000 == 0) {
		calc_fan = 1;
	}
}
