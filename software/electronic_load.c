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
#include "stm8s_clk.h"
#include "load.h"
#include "fan.h"
#include "adc.h"

#define OVERSAMPLING 2

#define MINUTE   6000
#define HOUR   360000

typedef struct {
	uint16_t charged_voltage;
	uint16_t cutoff_voltage;
	char     name[5];
} battery_voltage_t;

error_t           error           = ERROR_NONE;
char              error_msg[][5]  = {
	"UVP@",
	"OVP@",
	"OLP@",
	"OTP@",
	"PWR@",
};

battery_voltage_t voltages[] = {
	{
		150,
		 90,
		"NIMH"
	},
	{
		241,
		193,
		"LEAD"
	},
	{
		365,
		210,
		"LIFE"
	},
	{
		410,
		300,
		"LIPO"
	},
	{
		420,
		330,
		"LIIO"
	}
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

	// FAN
	//TIM3->ARRH   = 0x10;
	//TIM3->ARRL   = 0x00;
	TIM3->CCMR2  = TIM3_OCMODE_PWM1 | TIM3_CCMR_OCxPE;
	TIM3->CCER1  = TIM3_CCER1_CC2E;
	TIM3->PSCR   = TIM3_PRESCALER_1; // Prescaler of 1 gives 16 MHz / 2^16 = 244 Hz
	TIM3->CR1   |= TIM3_CR1_CEN | TIM3_CR1_ARPE;
	TIM3->CCR2H  = 0;
	TIM3->CCR2L  = 0;

	// I-SET
	// 28000 gives a frequency of about 571 Hz on 16 MHz
	TIM1->ARRH   = 0x6D;
	TIM1->ARRL   = 0x60;
	TIM1->PSCRH  = 0;
	TIM1->PSCRL  = 0;

	TIM1->CCMR1  = TIM1_OCMODE_PWM1 | TIM1_CCMR_OCxPE;
	TIM1->CCER1  = TIM1_CCER1_CC1E;
	TIM1->CCR1H  = 0;
	TIM1->CCR1L  = 0;
	TIM1->CR1   |= TIM1_CR1_CEN | TIM1_CR1_ARPE;
	TIM1->BKR   |= TIM1_BKR_MOE;

	// 20000 gives an interrupt frequency of 100 Hz -> 10ms
	TIM2->ARRH   = 0x4E;
	TIM2->ARRL   = 0x20;
	TIM2->PSCR   = TIM2_PRESCALER_8;
	TIM2->IER    = TIM2_IER_UIE;
	TIM2->CR1   |= TIM2_CR1_CEN;

	// Unlock flash
	FLASH->DUKR = FLASH_RASS_KEY2;
	FLASH->DUKR = FLASH_RASS_KEY1;
	while (!(FLASH->IAPSR & FLASH_IAPSR_DUL));

	// Set option bytes because of buzzer, not working yet
	/*if(OPT->OPT2 != 0x80 || OPT->OPT3 != 0x08){
		FLASH->CR2 |= FLASH_CR2_OPT; // unlock option bytes for writing
 		FLASH->NCR2 &= (uint8_t)(~FLASH_NCR2_NOPT);
		while (!(FLASH->IAPSR & FLASH_IAPSR_DUL));

		OPT->OPT2 = 0x80; // enable LSI clock source
		OPT->NOPT2 = 0x7f;
		OPT->OPT3 = 0x08; // set PD4 as alternative buzzer output
		OPT->NOPT3 = 0xf7;
		while (!(FLASH->IAPSR & FLASH_IAPSR_EOP)); // wait for write finish

		FLASH->CR2 &= (uint8_t)(~FLASH_CR2_OPT);	// lock back
  		FLASH->NCR2 |= FLASH_NCR2_NOPT;
	}*/

	// Setup buzzer
	// For buzzer working you must set option bytes: AFR7 - port D4 alternate function = BEEP and LSI_EN - LSI clock enable as CPU clock source
	// You can do this e.g. from graphic STVP
	BEEP->CSR &= ~BEEP_CSR_BEEPEN; // Disable Buzzer
    BEEP->CSR &= ~BEEP_CSR_BEEPDIV; // Clear DIV register
    BEEP->CSR |= BEEP_CALIBRATION_DEFAULT; // Set register with default calibration
 	BEEP->CSR &= ~BEEP_CSR_BEEPSEL; // Clear SEL register
 	BEEP->CSR |= BEEP_FREQUENCY_2KHZ; // Set frequency of buzzer to 2kHz
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
	set_mode = read8(MEM_MODE);
	set_values[MODE_CC] = read16(MEM_CC);
	set_values[MODE_CW] = read16(MEM_CW);
	set_values[MODE_CR] = read16(MEM_CR);
	set_values[MODE_CV] = read16(MEM_CV);
	beeper_on = read8(MEM_BEEP);
	cutoff_active = read8(MEM_CUTO);
	cutoff_voltage = read16(MEM_CUTV);

	delay(300000);
	setBrightness(2, DP_BOT);
	setBrightness(2, DP_TOP);
	showText("BOOT", DP_TOP);
	printf("LOAD READY\n");
	__asm__ ("rim");

	if(beeper_on){
		BEEP->CSR |= BEEP_CSR_BEEPEN; // Enable buzzer
		delay10ms(20);
		BEEP->CSR &= ~BEEP_CSR_BEEPEN;
	}

	while (1) {
		uint32_t start_time;
		showMenu();
		GPIOE->ODR &= ~GPIO_PIN_5;
		running = 1;
		setFan();
		start_time = tenmillis;

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
				uint8_t s_var = (tenmillis / 500) % 5;
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
						timer = (tenmillis - start_time) / MINUTE * 100 + ((tenmillis - start_time) / 100) % 60;
						showNumber(timer, 2, DP_TOP);
				}
				//printf("%lu; %u; %u; %lu; %lu; %u\n", (tenmillis - start_time), set_current, voltage, mAmpere_seconds, mWatt_seconds, temperature);
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
				disp_write(digits[3], LED_RUN * ((tenmillis / 50) & 1), DP_BOT);
				if(((tenmillis / 50) & 1) && !(BEEP->CSR & BEEP_CSR_BEEPEN) && beeper_on){ // Toggle beeper
					BEEP->CSR |= BEEP_CSR_BEEPEN;
				}
				else if(!((tenmillis / 50) & 1) && (BEEP->CSR & BEEP_CSR_BEEPEN && beeper_on)){
					BEEP->CSR &= ~BEEP_CSR_BEEPEN;
				}
			}
			BEEP->CSR &= ~BEEP_CSR_BEEPEN; // Turn off beeper if you jump from err while it's ON
			error = ERROR_NONE;
			encoder_pressed = 0;
		}
		run_pressed = 0;
		disp_write(digits[3], 0, DP_BOT);
	}

/*
	while (1) {
		showNumber((uint16_t)tenmillis, 2, DP_TOP);
		while ((UART2->SR & (uint8_t)UART2_SR_RXNE)){
			rcvBuff[countRx++] = (char) getchar();
		}
		printf("%s", rcvBuff);*
	}
	while (1) {
		uint16_t pwm = 0;
		uint16_t ma = 1000;
		bool chng = 1;
		while (1) {
			if (encoder_val < 0) {
				if (set_values[set_mode])
					set_values[set_mode] -= 100;
				chng = 1;
			} else if (encoder_val > 0) {
				set_values[set_mode] += 100;
				//pwm += 100;
				chng = 1;
			}
			if (run_pressed) {
				if (GPIOE->ODR & GPIO_PIN_5) {
					GPIOE->ODR &= ~GPIO_PIN_5;
				} else {
					GPIOE->ODR |= GPIO_PIN_5;
				}
				run_pressed = 0;
			}
			//showNumber((10720 - analogRead(ADC1_CHANNEL_0) * 10) >> 3, 1, DP_TOP);
			showNumber(set_values[set_mode], 3, DP_TOP);
			//delay(300000);
			disp_write(digits[0], chars[GPIOC->IDR & GPIO_PIN_2], DP_BOT);
			if (chng) {
				//showNumber(pwm, 1, DP_TOP);
				uint32_t set_pwm = 2119;
				set_pwm *= ma;
				set_pwm += 60043;
				//pwm = I_PWM_FACTOR * ma + I_PWM_OFFSET;
				pwm = set_pwm / 1000;
				TIM1->CCR1H = pwm >> 8;
				TIM1->CCR1L = (uint8_t) pwm;
				//showNumber((TIM1->CCR1H << 8) | TIM1->CCR1L, 0, DP_TOP);
				showNumber(ma, 3, DP_TOP);
				showNumber(pwm, 0, DP_BOT);
				if (ma < 1000)
					showNumber(ma, 0, DP_BOT);
				else if (ma < 10000)
					showNumber(ma / 10, 2, DP_BOT);
				else if (ma < 100000)
					showNumber(ma / 100, 1, DP_BOT);
				*//*
				chng = 0;
				encoder_val = 0;
				printf("%d\n", pwm);
			}
			getVoltage();
			calcPWM();
			getTemp();
			//setFan();
		}
	}
	while (1) {
		bool chng = 0;
		if (encoder_val < 0) {
			if (!TIM1->CCR1L) TIM1->CCR1H--;
			TIM1->CCR1L--;
			chng = 1;
		} else if (encoder_val > 0) {
			if (TIM1->CCR1L == 0xFF) TIM1->CCR1H++;
			TIM1->CCR1L++;
			chng = 1;
		}
		if (chng) {
			showNumber((TIM1->CCR1H << 8) | TIM1->CCR1L, 0, DP_TOP);
			chng = 0;
			encoder_val = 0;
		}

	}
	delay(1500000);

	//GPIOC->ODR &= ~GPIO_PIN_1;
	GPIOE->ODR |= GPIO_PIN_5; // LOAD OFF

	disp_write(digits[0], chars[EXTI->CR1], DP_TOP);
	disp_write(digits[1], chars[EXTI->CR2], DP_TOP);
	disp_write(digits[2], chars[12], DP_TOP);
	disp_write(digits[3], chars[13], DP_TOP);
	disp_write(digits[0], 0, DP_BOT);
	disp_write(digits[1], 0, DP_BOT);
	disp_write(digits[2], 0, DP_BOT);
	disp_write(digits[3], 0, DP_BOT);

	//printf("test\nDies ist ein Test!");
	//EXTI->CR1 |= EXTI_SENSITIVITY_FALL_ONLY << 4; //FALL_ONLY

	GPIOD->ODR &= ~GPIO_PIN_0;
	__asm__ ("rim"); // interrupts enabled
	//while (1) {
	//	showMenu();
	//}
	while (1) {
		uint16_t v1 = analogRead12(ADC1_CHANNEL_2);
		showNumber(v1 / 10, 0, DP_TOP);
		showNumber(v1 % 10, 0, DP_BOT);
		delay(300000);
	}
	while (1) {
		uint16_t v1, v_ref, v2, v_load, v;
		v1 = analogRead12(ADC1_CHANNEL_1);
		v_load = 1.02217839986557 * v1 - 81.5878664441528;
		//v_load = 4.05175 * v1 / 4 - 64.7543;
		//GPIOD->ODR |= GPIO_PIN_0;
		v2 = analogRead12(ADC1_CHANNEL_2);
		v_ref = 0.891348658196074 * v2 - 80.4250357289787;
		//v_ref = 3.52991 * v2 / 4 - 64.78; // ADC2
		//GPIOD->ODR &= ~GPIO_PIN_0;
		v = 0;
		if (v1 > 20) {
			v = v_load;
			//if (v_ref >= v_load && v_ref < v_load + 100) {
				v = v_ref;
				disp_write(digits[2], chars[1], DP_BOT);
			//} else {
			//	disp_write(digits[2], chars[3], DP_BOT);
			//}
		}
		disp_write(digits[0], chars[v / 1000], DP_TOP);
		disp_write(digits[1], chars[(v / 100) % 10] | 0x80, DP_TOP);
		disp_write(digits[2], chars[(v / 10) % 10], DP_TOP);
		disp_write(digits[3], chars[v % 10], DP_TOP);
		delay(300000);
	}
*/
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
