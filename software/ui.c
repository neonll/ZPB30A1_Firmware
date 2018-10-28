#include "ui.h"
#include "tm1650.h"
#include "timer.h"
#include "eeprom.h"
#include "load.h"
#include "config.h"
#include "settings.h"
#include "inc/stm8s_gpio.h"

volatile int8_t encoder_val = 0;
volatile bool encoder_pressed = 0;
volatile bool run_pressed = 0;
uint8_t brightness[] = {0, 0};
bool option_changed = 0;
volatile bool redraw = 0;


void ui_timer()
{
	static uint16_t timer = 0;
	timer++;
	if (timer == F_SYSTICK/F_DISPLAY_REDRAW) {
		timer = 0;
		redraw = 1;
	}
}


char mode_units[][5] = {"AMPS",	"WATT",	"OHMS",	"VOLT"};
char mode_text[][4] = {"CC@","CW@","CR@","CV@"};
char on_off_text[][4] = {"OFF","ON@"};

uint16_t max_values[NUM_MODES] = {
	10000, // 10A
	60000, // 60W
	50000, // 50Ω
	28000, // 28V
};

//TODO: Correctly handle bouncing encoder
void GPIOB_Handler() __interrupt(4) {
	static uint8_t _encoder_dir = 0xFF;
	uint8_t cur = (GPIOB->IDR >> 4) & 3;
	if (cur == 0) {
		if (_encoder_dir == 2) {
			encoder_val++;
		} else if (_encoder_dir == 1) {
			encoder_val--;
		}
	}
	_encoder_dir = cur;
}

void GPIOC_Handler() __interrupt(5) {
	static uint8_t input_values = 0xFF;
	input_values &= ~GPIOC->IDR; // store changes (H->L) for buttons
	encoder_pressed = input_values & PINC_ENC_P;
	run_pressed = input_values & PINC_RUN_P;
	if (input_values & PINC_OL_DETECT) error = ERROR_OLP;
	input_values = GPIOC->IDR;
}

void showText(char text[], uint8_t display)
{
	for (uint8_t i=0; i<4; i++) {
		if (display == DP_TOP || i != 3) disp_char(i, text[i], 0, display);
	}
}

void showNumber(uint16_t num, uint8_t dot, uint8_t display)
{
	uint16_t maximum = (display == DP_TOP)?10000:1000;
	uint16_t digits = (display == DP_TOP)?4:3;
	while (num >= maximum) {
		num /= 10;
		dot--;
	}
	for (int8_t i=digits-1; i>=0; i--)
	{
		disp_char(i, num % 10 + '0', dot==(digits-1-i), display);
		num /= 10;
	}
}

//// TODO: Old code

uint8_t select(char *opts, uint8_t num_opts, uint8_t selected)
{
	uint8_t old_opt = 0;		//was -1
	while (1) {
		if(encoder_val < 0){if(!selected){selected=num_opts;}selected--;}
		else {if(encoder_val > 0){selected++;if(selected == num_opts)selected=0;}}
		if (selected != old_opt) {
			disp_write(digits[0], chars[*(opts + selected * 3 + 0) - CHAR_OFFSET], DP_BOT);
			disp_write(digits[1], chars[*(opts + selected * 3 + 1) - CHAR_OFFSET], DP_BOT);
			disp_write(digits[2], chars[*(opts + selected * 3 + 2) - CHAR_OFFSET], DP_BOT);
			old_opt = selected;
			encoder_val = 0;
		}
	}
}

void blinkDisplay(uint8_t disp)
{
	uint8_t dptop = disp == DP_TOP;
	//TODO: Cleanup
	if (brightness[dptop] != ((systick >> 5) & 1)) {
		brightness[dptop] = !brightness[dptop];
		disp_brightness(1 + brightness[dptop], disp);
	}
}

uint8_t change_u8(uint8_t var, uint8_t max)
{
	if (encoder_val < 0) {
		option_changed = 1;
		if (var) {
			return var - 1;
		} else {
			return max;
		}
	} else if (encoder_val > 0) {
		option_changed = 1;
		if (var < max) {
			return var + 1;
		} else {
			return 0;
		}
	}
	return var;
}

uint16_t change_u16(uint16_t var, uint16_t max, uint16_t inc)
{
	if (encoder_val < 0) {
		option_changed = 1;
		if (var > inc) {
			return var - inc;
		} else {
			return 0;
		}
	} else if (encoder_val > 0) {
		option_changed = 1;
		if (var + inc < max) {
			return var + inc;
		} else {
			return max;
		}
	}
	return var;
}

void selectMode(void)
{
	while (1) {
		blinkDisplay(DP_BOT);
		settings.mode = change_u8(settings.mode, 2);		//CV mode not supported yet
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showText(mode_text[settings.mode], DP_BOT);
		}
		if (encoder_pressed) {
			settings_update();
			return;
		}
	}
}

bool selectBool(bool val)
{
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_val) {
			val = !val;
			encoder_val = 0;
			showText(on_off_text[val], DP_BOT);
		}
		if (encoder_pressed) {
			return val;
		}
	}
}

uint16_t selectUInt16(uint16_t val, uint16_t max)
{
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				return val;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(val, 3, DP_BOT);

			if (settings.setpoints[settings.mode] >= 10000) {
				inc = 100;
			} else if (settings.setpoints[settings.mode] >= 1000) {
				inc = 10;
			} else {
				inc = 1;
			}
			if (!hl_opt) {
				inc *= 10;
			}
		}
		val = change_u16(val, max, inc);

	}
}

void selectValue(void)
{
	bool change = 0;
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	char opts[][5] = {"MODE", "VAL@", "SHDN", "CUTO", "BEEP"};
	showText(opts[settings.mode], DP_TOP);
	disp_write(digits[3], LED_HIGH, DP_BOT);
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				settings_update();
				return;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(settings.setpoints[settings.mode], 3, DP_BOT);

			/*if (settings.setpoints[settings.mode] >= 100000) {
				inc = 1000;
			} else */if (settings.setpoints[settings.mode] >= 10000) {
				inc = 100;
			} else if (settings.setpoints[settings.mode] >= 1000) {
				inc = 10;
			} else {
				inc = 1;
			}
			if (!hl_opt) {
				inc *= 10;
			}
		}
		settings.setpoints[settings.mode] = change_u16(settings.setpoints[settings.mode], 10000, inc);
	}
}

void selectCutoff(void) {
	bool change = 0;
	bool hl_opt = 0;
	uint16_t inc = 10;
	option_changed = 1;
	disp_write(digits[3], LED_HIGH, DP_BOT);
	while (1) {
		blinkDisplay(DP_BOT);
		if (encoder_pressed) {
			encoder_pressed = 0;
			if (!hl_opt) {
				hl_opt = 1;
				option_changed = 1;
				disp_write(digits[3], LED_LOW, DP_BOT);
			} else {
				settings_update();
				return;
			}
		}
		if (option_changed) {
			option_changed = 0;
			encoder_val = 0;
			showNumber(settings.cutoff_voltage / 10, 1, DP_BOT);
		}
		settings.cutoff_voltage = change_u16(settings.cutoff_voltage, 2900, hl_opt ? 10 : 100);
	}
}

void showMenu()
{
	uint8_t old_opt = 255, opt = 0;
	char opts[][5] = {"MODE", "VAL@", "SHDN", "CUTO", "BEEP"};

	//char *opts = "MODEVAL@SHDNCUTOBEEP";
	//select(opts, 5, 0);
	while (1) {
		if (encoder_val < 0) {
			if (opt) {
				opt--;
			} else {
				opt = 4;
			}
		} else {
			if (encoder_val > 0) {
				if (opt < 4) {
					opt++;
				} else {
					opt = 0;
				}
			}
		}
		blinkDisplay(DP_TOP);
		if (opt != old_opt) {
			showText(opts[opt], DP_TOP);
			switch (opt) {
				case 0:
					showText(mode_text[(uint8_t)settings.mode], DP_BOT);
					break;
				case 1:
					showNumber(settings.setpoints[(uint8_t)settings.mode], 3, DP_BOT);
					break;
				case 2:
					showText(on_off_text[settings.cutoff_enabled], DP_BOT);
					break;
				case 3:
					showNumber(settings.cutoff_voltage, 2, DP_BOT);
					break;
				case 4:
					showText(on_off_text[settings.beeper_enabled], DP_BOT);
					break;
			}
			old_opt = opt;
			encoder_val = 0;
		}
		if (encoder_pressed) {
			encoder_pressed = 0;
			disp_brightness(2, DP_TOP);
			switch (opt) {
				case 0:
					selectMode();
					break;
				case 1:
					showText(mode_units[settings.mode], DP_TOP);
					settings.setpoints[settings.mode] = selectUInt16(settings.setpoints[settings.mode], max_values[settings.mode]);
					break;
				case 2:
					settings.cutoff_enabled = selectBool(settings.cutoff_enabled);
					break;
				case 3:
					selectCutoff();
					break;
				case 4:
					settings.beeper_enabled = selectBool(settings.beeper_enabled);
					break;
			}
			settings_update();
			disp_brightness(2, DP_BOT);
			disp_write(digits[3], 0, DP_BOT);
			encoder_pressed = 0;
			run_pressed = 0;
			old_opt = 255;
		}
		if (run_pressed) {
			run_pressed = 0;
			return;
		}
	}
}
