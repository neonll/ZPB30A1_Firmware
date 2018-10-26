#ifndef _CONFIG_H
#define _CONFIG_H_

//F_xxx is in Hz
#define F_CPU 16000000L
#define F_SYSTICK 100
#define F_PWM 571L

#define BAUDR 115200L

#define F_DISPLAY_REDRAW 2
#define F_POWER_CALC 1
#define F_FAN 0.2

#define F_UI_AUTODISPLAY 0.2
#define F_BEEP_ERROR 2
#define F_BEEP_KHZ 1 // 1, 2 or 4

#define ADC_OVERSAMPLING 2
#endif
