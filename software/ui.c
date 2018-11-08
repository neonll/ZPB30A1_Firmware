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
#include "inc/stm8s_itc.h"

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

static volatile int8_t encoder_val = 0;
static volatile bool encoder_pressed = 0;
static volatile bool run_pressed = 0;
static uint8_t display_mode[] = {DISP_MODE_BRIGHT, DISP_MODE_BRIGHT};

#define MENU_STACK_DEPTH 5
static const MenuItem *menu_stack[MENU_STACK_DEPTH];
static uint8_t menu_subitem_index[MENU_STACK_DEPTH];
static uint8_t menu_stack_head = 0;

#define current_item menu_stack[menu_stack_head]
#define current_subitem_index menu_subitem_index[menu_stack_head]
#define current_subitem current_item->subitems[current_subitem_index]

static void ui_text(char text[], uint8_t display);
static void ui_number(uint16_t num, uint8_t dot, uint8_t display);
static void ui_push_item(MenuItem *item);
static void ui_pop_item();

static void ui_set_display_mode(display_mode_t mode, display_t disp)
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
	menu_stack_head = 0;
	ui_push_item(&menu_main);
}

static void ui_timer_redraw()
{
	static uint16_t timer = 0;
	timer++;
	if (timer == F_SYSTICK/F_DISPLAY_REDRAW) {
		timer = 0;
	}
}

static void ui_blink(uint8_t mode)
{
	for (uint8_t i=0; i<2; i++)
	{
		if (display_mode[i] & mode) {
			ui_set_display_mode(display_mode[i] ^ DISP_MODE_DIM, i);
		}
	}
}

static void ui_timer_blink()
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

void ui_error_handler(uint8_t event, const MenuItem *item)
{
	(void) item; //Unused
	if (event == EVENT_PREVIEW || event == EVENT_TIMER) return;
	const char msgs[][5] = {"", "UVP ", "OVP ", "LOAD", "TEMP", "PWR ", "TIME", "INT "};
	load_disable();
	ui_text("ERR", DP_BOT);
	ui_text(msgs[error], DP_TOP);
	ui_set_display_mode(DISP_MODE_DIM, DP_TOP);
	ui_set_display_mode(DISP_MODE_DIM, DP_BOT);
	if (event == EVENT_ENCODER_BUTTON) {
		error = ERROR_NONE;
		ui_pop_item();
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
	if (error && current_item != &menu_error) {
		ui_push_item(&menu_error);
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
	current_item->handler(EVENT_TIMER, current_item);
	encoder_val = 0;
	run_pressed = 0;
	encoder_pressed = 0;
}

static void ui_text(const char *text, uint8_t display)
{
	for (uint8_t i=0; i<4; i++) {
		if (display == DP_TOP || i != 3) disp_char(i, text[i], 0, display);
	}
}

static void ui_number(uint16_t num, uint8_t dot, uint8_t display)
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

static void ui_leds(uint8_t leds)
{
	uint8_t run_led = load_active ? LED_RUN : 0;
	disp_leds(leds | run_led);
}

static uint8_t ui_num_subitem(const MenuItem *item)
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

static void ui_pop_item()
{
	if (menu_stack_head != 0) {
		menu_stack_head--;
	}
	ui_leds(0);
	settings_update(); //Store change value to eeprom
	current_item->handler(EVENT_RETURN, current_item);
}

/* This function must only be called for the currently active item. */
static void ui_select(uint8_t event, const MenuItem *item, uint8_t display)
{
	uint8_t display2 = display==DP_TOP?DP_BOT:DP_TOP;
	bool output = event & (EVENT_BITMASK_MENU | EVENT_BITMASK_ENCODER);
	if (event & EVENT_BITMASK_MENU) {
		ui_set_display_mode(DISP_MODE_BLINK_FAST, display);
		ui_set_display_mode(DISP_MODE_DIM, display2);
	}
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
	if (event == EVENT_RUN_BUTTON && menu_stack_head == 0)
	{
		//Main menu + Run button => turn on load
		ui_push_item(&menu_active);
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

static uint8_t ui_find_active_subitem(const MenuItem *item)
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
/* Edit numeric values. This function encapsulates almost all the logic
   except setting the secondard display's value and selecting the LEDs. */
void ui_edit_value_internal(uint8_t event, const NumericEdit *edit, uint8_t leds)
{
	static uint16_t value;
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
		ui_leds(leds | LED_DIGIT1);
		value = *edit->var;
		current_subitem_index = 0;
		break;
	case EVENT_RUN_BUTTON:
		pop = true;
		break;
	case EVENT_ENCODER_BUTTON:
		if (current_subitem_index == 0) {
			current_subitem_index = 1;
			ui_leds(leds | LED_DIGIT2);
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

void ui_edit_value(uint8_t event, const MenuItem *item)
{
	const NumericEdit *edit = item->data;
	ui_edit_value_internal(event, edit, item->value);
}

void ui_edit_setpoint(uint8_t event, const MenuItem *item)
{
	(void) item; //unused
	const NumericEdit *edit = 0;
	const char *label = 0;
	uint8_t leds = 0;
	switch (settings.mode) {
		case MODE_CC:
			edit = &menu_value_edit_CC;
			label = "AMP ";
			leds = LED_A;
			break;
		case MODE_CV:
			edit = &menu_value_edit_CV;
			label = "VOLT";
			leds = LED_V;
			break;
		case MODE_CR:
			edit = &menu_value_edit_CR;
			label = "OHM ";
			break;
		case MODE_CW:
			edit = &menu_value_edit_CW;
			label = "WATT";
			break;
		default:
			edit = 0;
			label = "===";
			break;
	}
	if (!(event & EVENT_PREVIEW)) ui_text(label, DP_TOP);
	if (edit) ui_edit_value_internal(event, edit, leds);
}

void ui_active(uint8_t event, const MenuItem *item)
{
	(void) item; //unused
	if (event & EVENT_PREVIEW) return; //Unsupported
	if (event == EVENT_ENTER || event == EVENT_RETURN) {
		load_enable();
		ui_leds(0);
		ui_set_display_mode(DISP_MODE_DIM, DP_TOP);
		ui_set_display_mode(DISP_MODE_DIM, DP_BOT);
		ui_text("RUN ", DP_TOP);
		ui_text("RUN ", DP_BOT);
	}
	if (event == EVENT_RUN_BUTTON) {
		load_disable();
		ui_pop_item();
		return;
	}
}

//TODO: Correctly handle bouncing encoder
void ui_encoder_irq() __interrupt(ITC_IRQ_PORTB)
{
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

void ui_button_irq() __interrupt(ITC_IRQ_PORTC)
{
	static uint8_t input_values = 0xFF;
	input_values &= ~GPIOC->IDR; // store changes (H->L) for buttons
	encoder_pressed |= input_values & PINC_ENC_P;
	run_pressed |= input_values & PINC_RUN_P;
	input_values = GPIOC->IDR;
}
