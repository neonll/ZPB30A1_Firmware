#include "config.h"
#include "uart.h"
#include "utils.h"
#include "ui.h"
#include "stm8s_conf.h"
#include "stdio.h"
#include "tm1650.h"
#include "eeprom.h"
#include "timer.h"
#include "todo.h"
#include "settings.h"
#include "inc/stm8s_clk.h"
#include "load.h"
#include "fan.h"
#include "adc.h"
#include "beeper.h"

#define OVERSAMPLING 2

#define MINUTE   60
#define HOUR   3600

error_t           error           = ERROR_NONE;
char              error_msg[][5]  = {
	"UVP@",
	"OVP@",
	"OLP@",
	"OTP@",
	"PWR@",
};


void clock_init()
{
	CLK->CKDIVR = CLK_PRESCALER_HSIDIV1;
	CLK->ICKR |= CLK_ICKR_LSIEN; // Low speed RC for beeper
	CLK->ECKR |= CLK_ECKR_HSEEN; // Crystal oscillator
	while ((CLK->ECKR & CLK_FLAG_HSERDY) == 0); // wait for crystal startup
	CLK->SWR = CLK_SOURCE_HSE;
	//CLK->SWCR = CLK_SWCR_SWEN;*/
	CLK->CKDIVR = 0;
}

void gpio_init()
{
	// port B
	#define PINB_ENC_A (1u<<5)
	#define PINB_ENC_B (1u<<4)
	GPIOB->CR1 = PINB_ENC_A | PINB_ENC_B; // Pullup
	GPIOB->CR2 = PINB_ENC_A | PINB_ENC_B; // Irq

	//port C
	#define PINC_I_SET (1u<<1)
	#define PINC_OL_DETECT (1u<<2)
	#define PINC_ENC_P (1u<<3)
	#define PINC_RUN_P (1u<<4)
	#define PINC_SCL (1u<<5)
	#define PINC_SDA1 (1u<<6)
	#define PINC_SDA2 (1u<<7)
	GPIOC->DDR = PINC_I_SET | PINC_SCL | PINC_SDA1 | PINC_SDA2;
	//TODO: In the original version SDA is configured as push-pull
	// but that doesn't make sense
	GPIOC->CR1 = PINC_I_SET | PINC_SCL | // push pull
				 PINC_OL_DETECT | PINC_ENC_P | PINC_RUN_P; // pullup
	GPIOC->CR2 = PINC_OL_DETECT | PINC_ENC_P | PINC_RUN_P |  // irq
				 PINC_SCL | PINC_SDA1 | PINC_SDA2; // 10 MHz

	// port D
	#define PIND_FAN (1u<<0)
	#define PIND_SWIM (1u<<1)
	#define PIND_BUS_F (1u<<2)
	#define PIND_V_OK (1u<<3)
	#define PIND_BEEPER (1u<<4)
	#define PIND_TX (1u<<5)
	#define PIND_RX (1u<<6)
	#define PIND_TLI (1u<<7)
	GPIOD->DDR = PIND_FAN | PIND_BUS_F | PIND_BEEPER | PIND_TX;
	GPIOD->CR1 = PIND_FAN | PIND_BUS_F | PIND_BEEPER | PIND_TX | // push pull
				 PIND_V_OK | PIND_TLI; // pullup
	GPIOD->CR2 = PIND_V_OK | PIND_TLI; // irq

	// port E
	#define PINE_ENABLE (1<<5)
	GPIOE->ODR = PINE_ENABLE; // load off
	GPIOE->DDR = PINE_ENABLE; // pullup

	EXTI->CR2    = EXTI_SENSITIVITY_FALL_ONLY; // TLI
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 2; // GPIOB
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 4; // GPIOC
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 6; // GPIOD
}

void setup(void)
{
	clock_init();
	gpio_init();
	adc_init();
	uart_init();
	systick_init();
	load_init();
	beeper_init();
	settings_init();
}

uint16_t analogRead(ADC1_Channel_TypeDef ch) {
	uint8_t adcH, adcL;
	ADC1->CSR &= (uint8_t)(~ADC1_CSR_CH);
	ADC1->CSR |= (uint8_t)(ch);

	ADC1->CR1 |= ADC1_CR1_ADON;
	while (!(ADC1->CSR & ADC1_IT_EOC));
	adcL = ADC1->DRL;
	adcH = ADC1->DRH;
	ADC1->CSR &= ~ADC1_IT_EOC;
	return (adcL | (adcH << 8));
}

uint16_t analogRead12(ADC1_Channel_TypeDef ch) {
	uint16_t val = 0;
	uint8_t i;
	for (i = 0; i < 4 * OVERSAMPLING; i++) {
		val += analogRead(ch);
	}
	return val / OVERSAMPLING;
}

void getTemp(void) {
	uint16_t tmp = (10720 - analogRead(ADC1_CHANNEL_0) * 10) >> 3;
	temperature = tmp;
}

void getVoltage(void) {
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

void calcPWM(void) {
	uint16_t pwm;
	uint32_t current;
	switch (set_mode) {
		case MODE_CC:
			current = set_values[MODE_CC];
			break;
		case MODE_CW: // I = P / U
			current = set_values[MODE_CW];
			current *= 100; //voltage is in V/100
			current /= voltage;
			break;
		case MODE_CR: // I = U / R
			current = ((uint32_t) voltage * 10000) / set_values[MODE_CR]; //R in 0,001 Ohm
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


void main(void) {
	setup();
	delay(300000);
	setBrightness(2, DP_BOT);
	setBrightness(2, DP_TOP);
	showText("BOOT", DP_TOP);
	printf("LOAD READY\n");
	__asm__ ("rim");

	beeper_on();
	delay10ms(20);
	beeper_off();

	while (1) {
		uint32_t start_time;
		showMenu();
		GPIOE->ODR &= ~GPIO_PIN_5;
		running = 1;
		setFan();
		start_time = systick;

		while (!run_pressed && error == ERROR_NONE) {
			getVoltage();
			if (voltage > 3000) {
				error = ERROR_OVP;
				break;
			}
			if (cutoff_active && voltage < cutoff_voltage) {
				error = ERROR_UVP;
				break;
			}
			calcPWM();
			if (redraw) {
				uint8_t s_var = (systick / (uint32_t)(F_SYSTICK / F_UI_AUTODISPLAY)) % 5;
				uint16_t timer;
				switch (s_var) {
					case 0:
						showNumber(voltage, 2, DP_TOP);
						break;
					case 1:
						showNumber(mAmpere_seconds / 3600, 3, DP_TOP);
						break;
					case 2:
						showNumber(mWatt_seconds / 3600, 3, DP_TOP);
						break;
					case 3:
						showNumber(temperature, 1, DP_TOP);
						break;
					case 4:
						//Convert to number for display M.SS
						//TODO: Switch to H.MM for longer times
						timer = (systick - start_time) / MINUTE / F_SYSTICK * 100 + ((systick - start_time) / F_SYSTICK) % MINUTE;
						showNumber(timer, 2, DP_TOP);
				}
				//printf("%lu; %u; %u; %lu; %lu; %u\n", (systick - start_time), set_current, voltage, mAmpere_seconds, mWatt_seconds, temperature);
				//showNumber(analogRead(ADC1_CHANNEL_0), 4, DP_TOP);
				if(logging){
					printf("$%u;%u;%lu;%lu;%u\r\n", set_current, voltage, mAmpere_seconds, mWatt_seconds, temperature);
				}

				showNumber(set_current, 3, DP_BOT);
				disp_write(digits[3], LED_RUN | (1 << s_var), DP_BOT);
				redraw = 0;
			}
			tempFan();
		}
		running = 0;
		GPIOE->ODR |= GPIO_PIN_5;
		TIM1->CCR1H = 0; // turn off PWM, to avoid current peak on next start
		TIM1->CCR1L = 0;
		encoder_pressed = 0;
		if (error != ERROR_NONE) {
			showText("ERR", DP_BOT);
			showText(error_msg[error - 1], DP_TOP);
			while (!encoder_pressed) {
				tempFan();
				//showNumber(temperature, 1, DP_TOP);
				disp_write(digits[3], LED_RUN * ((systick / 50) & 1), DP_BOT);
				//TODO: Magic numbers
				if((systick / 25) & 1) {
					beeper_toggle();
				}
			}
			beeper_off();
			//TODO: Keep fan regulator active in error mode
			error = ERROR_NONE;
			encoder_pressed = 0;
		}
		run_pressed = 0;
		disp_write(digits[3], 0, DP_BOT);
	}
}

//Voltage OK interrupt
void GPIOD_Handler() __interrupt(6) {
}

/* If you have multiple source files in your project, interrupt service routines
 can be present in any of them, but a prototype of the isr MUST be present or
 included in the file that contains the function main. */
void GPIOB_Handler() __interrupt(4);
void GPIOC_Handler() __interrupt(5);
void UART2_RX_IRQHandler() __interrupt(21);
// void GPIOD_Handler() __interrupt(6);
void TIM2_UPD_OVF_Handler() __interrupt(13);
