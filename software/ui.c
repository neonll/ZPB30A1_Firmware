#include "ui.h"
#include "tm1650.h"
#include "timer.h"
#include "eeprom.h"
#include "load.h"
#include "config.h"
#include "settings.h"
#include "menu_items.h"
#include "beeper.h"
#include "utils.h"
#include "stdio.h" //Debugging only
#include "inc/stm8s_gpio.h"

typedef enum {
	/* Bitmask:
	Bit 0: Brightness
	Bit 1: Blink fast
	Bit 2: Blink slow
	Setting bit 1 and 2 at the same time results in undefinded behaviour.
	*/
	DISP_MODE_BRIGHT       = 0b000,
	DISP_MODE_DIM          = 0b001,
	DISP_MODE_BLINK_FAST   = 0b010,
	DISP_MODE_BLINK_SLOW   = 0b100,
} display_mode_t;

typedef enum {
	EVENT_BITMASK_MENU 			= 0b11,
	EVENT_ENTER 				= 0b10,
	EVENT_RETURN				= 0b11,
	EVENT_BITMASK_ENCODER 		= 0b11100,
	EVENT_ENCODER_UP 			= 0b10000,
	EVENT_ENCODER_DOWN 			= 0b10100,
	EVENT_ENCODER_BUTTON		= 0b11000,
	EVENT_RUN_BUTTON			= 0b11100,
	EVENT_TIMER					= 0b100000,
	EVENT_PREVIEW 				= 0b1000000,
} ui_event_t;

volatile int8_t encoder_val = 0;
volatile bool encoder_pressed = 0;
volatile bool run_pressed = 0;
static uint8_t display_mode[] = {DISP_MODE_BRIGHT, DISP_MODE_BRIGHT};
#define MENU_STACK_DEPTH 5
const MenuItem *menu_stack[MENU_STACK_DEPTH];
uint8_t menu_subitem_index[MENU_STACK_DEPTH];
uint8_t menu_stack_head = 0;
#define current_item menu_stack[menu_stack_head]
#define current_subitem_index menu_subitem_index[menu_stack_head]
#define current_subitem current_item->subitems[current_subitem_index]


char mode_units[][5] = {"AMPS",	"WATT",	"OHMS",	"VOLT"};
char on_off_text[][4] = {"OFF","ON "};

static void ui_text(char text[], uint8_t display);
static void ui_number(uint16_t num, uint8_t dot, uint8_t display);
static void ui_push_item(MenuItem *item);

void ui_set_display_mode(display_mode_t mode, display_t disp)
{
	display_mode[disp] = mode;
	if (mode & DISP_MODE_DIM) {
		disp_brightness(BRIGHTNESS_DIM, disp);
	} else {
		disp_brightness(BRIGHTNESS_BRIGHT, disp);
	}
}

void ui_init()
{
	ui_set_display_mode(DISP_MODE_BRIGHT, DP_TOP);
	ui_set_display_mode(DISP_MODE_BRIGHT, DP_BOT);
	ui_text("BOOT", DP_TOP);
	ui_text("888", DP_BOT);
	menu_stack_head = 0;
	ui_push_item(&menu_main);
}

void ui_timer_redraw()
{
	static uint16_t timer = 0;
	timer++;
	if (timer == F_SYSTICK/F_DISPLAY_REDRAW) {
		timer = 0;
	}
}

void ui_blink(uint8_t mode)
{
	for (uint8_t i=0; i<2; i++)
	{
		if (display_mode[i] & mode) {
			ui_set_display_mode(display_mode[i] ^ DISP_MODE_DIM, i);
		}
	}
}

void ui_timer_blink()
{
	static uint16_t slow_timer = 0;
	static uint16_t fast_timer = 0;
	slow_timer++;
	if (slow_timer == F_SYSTICK/F_DISPLAY_BLINK_SLOW) {
		slow_timer = 0;
		ui_blink(DISP_MODE_BLINK_SLOW);
	}

	fast_timer++;
	if (fast_timer == F_SYSTICK/F_DISPLAY_BLINK_FAST) {
		fast_timer = 0;
		ui_blink(DISP_MODE_BLINK_FAST);
	}
}

void ui_timer()
{
	ui_timer_redraw();
	ui_timer_blink();
	if (encoder_val) {
		beeper_on();
		_delay_us(30);
		beeper_off();
	}
	if (encoder_val > 0) {
		current_item->handler(EVENT_ENCODER_UP, current_item);
	}
	if (encoder_val < 0) {
		current_item->handler(EVENT_ENCODER_DOWN, current_item);
	}
	if (encoder_pressed) {
		current_item->handler(EVENT_ENCODER_BUTTON, current_item);
	}
	if (run_pressed) {
		current_item->handler(EVENT_RUN_BUTTON, current_item);
	}
	encoder_val = 0;
	run_pressed = 0;
	encoder_pressed = 0;
}

void ui_text(char *text, uint8_t display)
{
	for (uint8_t i=0; i<4; i++) {
		if (display == DP_TOP || i != 3) disp_char(i, text[i], 0, display);
	}
}

void ui_number(uint16_t num, uint8_t dot, uint8_t display)
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

uint8_t ui_num_subitem(const MenuItem *item)
{
	uint8_t max_ = 0;
	const MenuItem **p = item->subitems;
	while (*p++) max_++;
	return max_;
}


// Menu item handlers
static void ui_push_item(const MenuItem *item)
{
	if (menu_stack_head != 0 || menu_stack[0] != 0) {
		menu_stack_head++; //First push is for the main menu
	}
	current_item = item;
	current_subitem_index = 0;
	item->handler(EVENT_ENTER, item);
}

void ui_pop_item()
{
	if (menu_stack_head != 0) {
		menu_stack_head--;
	}
	disp_leds(0);
	settings_update(); //Store change value to eeprom
	current_item->handler(EVENT_RETURN, current_item);
}

/* This function must only be called for the currently active item. */
void ui_handle_subitems(uint8_t event, const MenuItem *item)
{
	if (event == EVENT_ENCODER_UP) {
		if (current_subitem_index < ui_num_subitem(item) - 1) {
			current_subitem_index++;
		} else {
			current_subitem_index = 0;
		}
	}
	if (event == EVENT_ENCODER_DOWN) {
		if (current_subitem_index > 0) {
			current_subitem_index--;
		} else {
			current_subitem_index = ui_num_subitem(item) - 1;
		}
	}
}

void ui_select(uint8_t event, const MenuItem *item, uint8_t display)
{
	uint8_t display2 = display==DP_TOP?DP_BOT:DP_TOP;
	bool output = event & (EVENT_BITMASK_MENU | EVENT_BITMASK_ENCODER);
	if (event & EVENT_BITMASK_MENU) {
		ui_set_display_mode(DISP_MODE_BLINK_FAST, display);
		ui_set_display_mode(DISP_MODE_DIM, display2);
	}
	ui_handle_subitems(event, item);
	if (output) {
		ui_text(current_subitem->caption, display);
	}
	if (event == EVENT_RUN_BUTTON) {
		ui_pop_item();
	}
}


/** Allows selecting a menu item in the top display.
    The child handler is called to update the bottom display. */
void ui_submenu(uint8_t event, const MenuItem *item)
{
	if (event & EVENT_PREVIEW) {
		// Show nothing in bottom display as preview
		ui_text("   ", DP_BOT);
		return;
	}

	ui_select(event, item, DP_TOP);
	if (current_subitem->handler) {
		current_subitem->handler(event | EVENT_PREVIEW, current_subitem);
	} else {
		ui_text("===", DP_BOT); //Show warning that no handler is defined (makes no sense for ui_submenu!)
	}
	if (event == EVENT_ENCODER_BUTTON) {
		ui_push_item(current_subitem);
	}
}

uint8_t ui_find_active_subitem(const MenuItem *item)
{
	uint8_t n = ui_num_subitem(item);
	uint8_t value = *((uint8_t*)item->data);
	for (uint8_t i=0; i<n; i++) {
		if (item->subitems[i]->value == value) {
			return i;
		}
	}
	// Item not found => use first subitem as default value!
	return 0;
}

/** Allows selecting an item in the bottom menu.
    If the child element has an event handler it is called on selection.
	Otherwise the parent's data element is expected to point to a uint8_t
	variable which is set to the child's data element. */
void ui_select_item(uint8_t event, const MenuItem *item)
{
	if (event & EVENT_PREVIEW) {
		// Show selected value in bottom display as preview
		ui_text(item->subitems[ui_find_active_subitem(item)]->caption, DP_BOT);
		return;
	}
	if (event == EVENT_ENTER) {
		current_subitem_index = ui_find_active_subitem(item);
	}
	ui_select(event, item, DP_BOT);

	if (event == EVENT_ENCODER_BUTTON) {
		/* If a handler is available use it to set the value. */
		if (current_subitem->handler) {
			current_subitem->handler(EVENT_ENTER, current_subitem);
		} else {
			/* No handler => just set the value ourselves. */
			uint8_t *p = (uint8_t*)item->data;
			*p = current_subitem->value;
			ui_pop_item();
		}
	}
}

//TODO: Change step size depeding on encoder speed
void ui_edit_value(uint8_t event, const MenuItem *item)
{
	static uint16_t value;
	const NumericEdit *edit = item->data;
	uint8_t display = DP_BOT, display2 = DP_TOP;
	if (event & EVENT_PREVIEW) {
		ui_number(*edit->var, edit->dot_offset, display);
		return;
	}

	uint16_t inc;
	if (value >= 10000) {
		inc = 100;
	} else if (value >= 1000) {
		inc = 10;
	} else {
		inc = 1;
	}
	if (current_subitem_index == 0) {
		inc *= 10;
	}

	bool pop = false;

	switch (event) {
	case EVENT_ENTER:
		ui_set_display_mode(DISP_MODE_BRIGHT, display);
		ui_set_display_mode(DISP_MODE_DIM, display2);
		disp_leds(item->value | LED_DIGIT1);
		value = *edit->var;
		current_subitem_index = 0;
		break;
	case EVENT_RUN_BUTTON:
		pop = true;
		break;
	case EVENT_ENCODER_BUTTON:
		if (current_subitem_index == 0) {
			current_subitem_index = 1;
			disp_leds(item->value | LED_DIGIT2);
		} else {
			*edit->var = value;
			pop = true;
		}
		break;
	case EVENT_ENCODER_UP:
		if (value < edit->max - inc) {
			 value += inc;
		 } else {
			 value = edit->max;
		 }
		break;
	case EVENT_ENCODER_DOWN:
		if (value > edit->min + inc) {
			value -= inc;
		} else {
			value = edit->min;
		}
		break;
	}
	bool output = !pop && event & (EVENT_BITMASK_MENU | EVENT_BITMASK_ENCODER);
	if (output) {
		ui_number(value, edit->dot_offset, display);
	}
	if (pop) {
		// Pop must be the last action to avoid overwriting the display
		ui_pop_item();
	}
}

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
	encoder_pressed |= input_values & PINC_ENC_P;
	run_pressed |= input_values & PINC_RUN_P;
	if (input_values & PINC_OL_DETECT) error = ERROR_OLP;
	input_values = GPIOC->IDR;
}
