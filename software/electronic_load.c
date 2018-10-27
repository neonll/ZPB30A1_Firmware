#include "config.h"
#include "uart.h"
#include "utils.h"
#include "ui.h"
#include "stdio.h"
#include "tm1650.h"
#include "eeprom.h"
#include "timer.h"
#include "settings.h"
#include "load.h"
#include "fan.h"
#include "adc.h"
#include "beeper.h"
#include "inc/stm8s_clk.h"
#include "inc/stm8s_exti.h"

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
	GPIOB->CR1 = PINB_ENC_A | PINB_ENC_B; // Pullup
	GPIOB->CR2 = PINB_ENC_A | PINB_ENC_B; // Irq

	GPIOC->DDR = PINC_I_SET | PINC_SCL | PINC_SDA1 | PINC_SDA2;
	GPIOC->CR1 = PINC_I_SET | PINC_SCL | // push pull
				 PINC_OL_DETECT | PINC_ENC_P | PINC_RUN_P; // pullup
	GPIOC->CR2 = PINC_OL_DETECT | PINC_ENC_P | PINC_RUN_P |  // irq
				 PINC_SCL | PINC_SDA1 | PINC_SDA2; // 10 MHz


	GPIOD->DDR = PIND_FAN | PIND_BUS_F | PIND_BEEPER | PIND_TX;
	GPIOD->CR1 = PIND_FAN | PIND_BUS_F | PIND_BEEPER | PIND_TX | // push pull
				 PIND_V_OK | PIND_TLI; // pullup
	GPIOD->CR2 = PIND_V_OK | PIND_TLI; // irq

	GPIOE->ODR = PINE_ENABLE; // load off
	GPIOE->DDR = PINE_ENABLE; // pullup

	EXTI->CR2    = EXTI_SENSITIVITY_FALL_ONLY; // TLI
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 2; // GPIOB
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 4; // GPIOC
	EXTI->CR1   |= EXTI_SENSITIVITY_RISE_FALL << 6; // GPIOD
}



void main(void) {
	clock_init();
	gpio_init();
	adc_init();
	uart_init();
	systick_init();
	load_init();
	beeper_init();
	fan_init();
	settings_init();

	delay(300000);
	setBrightness(2, DP_BOT);
	setBrightness(2, DP_TOP);
	showText("BOOT", DP_TOP);
	__asm__ ("rim");

	beeper_on();
	delay10ms(20);
	beeper_off();
	systick_flag = 0; // Clear any overflows up to this point
	while (1) {
		if (systick_flag & SYSTICK_OVERFLOW)
		{
			load_disable();
			//TODO: Generalize error handler
			showText("SYST", DP_TOP);
			showText("ERR", DP_BOT);
			continue;  // do not perform any other actions.
		}
		if (systick_flag & SYSTICK_COUNT) {
			fan_timer();
			ui_timer();
			load_timer();
			systick_flag &= ~ SYSTICK_COUNT;
		}

		/////////////////////// TODO: OLD code
		uint32_t start_time;
		showMenu();
		load_disable();
		load_active = 1;
		start_time = systick;

		while (!run_pressed && error == ERROR_NONE) {
			getVoltage();
			if (voltage > 3000) {
				error = ERROR_OVP;
				break;
			}
			if (settings.cutoff_enabled && voltage < settings.cutoff_voltage) {
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
				printf("$%u;%u;%lu;%lu;%u\r\n", set_current, voltage, mAmpere_seconds, mWatt_seconds, temperature);

				showNumber(set_current, 3, DP_BOT);
				disp_write(digits[3], LED_RUN | (1 << s_var), DP_BOT);
				redraw = 0;
			}
		}
		load_active = 0;
		GPIOE->ODR |= PINE_ENABLE; //TODO: load_enable();
		TIM1->CCR1H = 0; // turn off PWM, to avoid current peak on next start
		TIM1->CCR1L = 0;
		encoder_pressed = 0;
		if (error != ERROR_NONE) {
			showText("ERR", DP_BOT);
			showText(error_msg[error - 1], DP_TOP);
			while (!encoder_pressed) {
				//showNumber(temperature, 1, DP_TOP);
				disp_write(digits[3], LED_RUN * ((systick / 50) & 1), DP_BOT);
				//TODO: Magic numbers
				if ((systick / (F_SYSTICK / F_BEEP_ERROR)) & 1) {
					beeper_on();
				} else {
					beeper_off();
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
