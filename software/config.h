#ifndef _CONFIG_H
#define _CONFIG_H_

//F_xxx is in Hz
#define F_CPU 16000000L
#define F_SYSTICK 100
#define F_PWM 571L

#define BAUDR 115200L

#define F_DISPLAY_REDRAW 2
#define F_DISPLAY_BLINK_SLOW 3
#define F_DISPLAY_BLINK_FAST 15

#define F_POWER_CALC 1
#define F_FAN 0.2

#define F_UI_AUTODISPLAY 0.2
#define F_BEEP_ERROR 2
#define F_BEEP_KHZ 1 // 1, 2 or 4

#define VOLT_MIN 1000 //mV
#define VOLT_MAX 30000 //mV
#define CUR_MIN 200 //mA
#define CUR_MAX 10000 //mA
#define POW_MIN 0 //mW
#define POW_MAX 60000 //mW
#define R_MIN 100 //mOhm
#define R_MAX 60000 //mOhm

#define ADC_OVERSAMPLING 2


// port B
#define PINB_ENC_A (1u<<5)
#define PINB_ENC_B (1u<<4)

//port C
#define PINC_I_SET (1u<<1)
#define PINC_OL_DETECT (1u<<2)
#define PINC_ENC_P (1u<<3)
#define PINC_RUN_P (1u<<4)
#define PINC_SCL (1u<<5)
#define PINC_SDA1 (1u<<6)
#define PINC_SDA2 (1u<<7)

// port D
#define PIND_FAN (1u<<0)
#define PIND_SWIM (1u<<1)
#define PIND_BUS_F (1u<<2)
#define PIND_V_OK (1u<<3)
#define PIND_BEEPER (1u<<4)
#define PIND_TX (1u<<5)
#define PIND_RX (1u<<6)
#define PIND_TLI (1u<<7)

// port E
#define PINE_ENABLE (1<<5)


#define GPIO_DISPLAY GPIOC
#define DP_TOP_PIN PINC_SDA2
#define DP_BOT_PIN PINC_SDA1
#define LED_V       0x01
#define LED_AH      0x02
#define LED_WH      0x04
#define LED_A       0x08
#define LED_RUN     0x10
#define LED_DIGIT1  0x20
#define LED_DIGIT2  0x40
#endif
