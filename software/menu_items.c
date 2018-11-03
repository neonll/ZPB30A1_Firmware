#include "menu_items.h"
#include "ui.h"
#include "settings.h"
#include "config.h"

const MenuItem menu_main;
    static const MenuItem menu_mode;
        static const MenuItem menu_mode_CC;
        static const MenuItem menu_mode_CV;
        static const MenuItem menu_mode_R;
        static const MenuItem menu_mode_P;
    static const MenuItem menu_value;
    static const MenuItem menu_beep;
    static const MenuItem menu_cutoff;
        static const MenuItem menu_cutoff_enabled;
        static const MenuItem menu_cutoff_value;

static const MenuItem menu_on = {
    .caption = "ON ",
    .value = 1
};

static const MenuItem menu_off = {
    .caption = "OFF",
    .value = 0
};


const MenuItem menu_main = {
    .caption = "Main",
    .handler = &ui_submenu,
    .subitems = { &menu_mode, &menu_value, &menu_beep, &menu_cutoff, 0}
};

static const MenuItem menu_mode = {
    .caption = "MODE",
    .handler = &ui_select_item,
    .data = &settings.mode,
    .subitems = {&menu_mode_CV,  &menu_mode_CC, &menu_mode_R, &menu_mode_P, 0}
};

static const MenuItem menu_mode_CC = {
    .caption = "CC ",
    .value = MODE_CC
};

static const MenuItem menu_mode_CV = {
    .caption = "CV ",
    .value = MODE_CV
};

static const MenuItem menu_mode_R = {
    .caption = "CR ",
    .value = MODE_CR
};

static const MenuItem menu_mode_P = {
    .caption = "CW ",
    .value = MODE_CW
};

const NumericEdit menu_value_edit_CC = {
    .var = &settings.setpoints[MODE_CC],
    .min = CUR_MIN,
    .max = CUR_MAX,
    .dot_offset = 3,
};

const NumericEdit menu_value_edit_CV = {
    .var = &settings.setpoints[MODE_CV],
    .min = VOLT_MIN,
    .max = VOLT_MAX,
    .dot_offset = 3,
};

const NumericEdit menu_value_edit_CR = {
    .var = &settings.setpoints[MODE_CR],
    .min = R_MIN,
    .max = R_MAX,
    .dot_offset = 3,
};

const NumericEdit menu_value_edit_CW = {
    .var = &settings.setpoints[MODE_CW],
    .min = POW_MIN,
    .max = POW_MAX,
    .dot_offset = 3,
};

static const MenuItem menu_value = {
    .caption = "VAL ",
    .handler = &ui_edit_setpoint,
};

static const MenuItem menu_beep = {
    .caption = "BEEP",
    .handler = &ui_select_item,
    .data = &settings.beeper_enabled,
    .subitems = {&menu_on,  &menu_off,  0}
};

static const MenuItem menu_cutoff = {
    .caption = "CUTO",
    .handler = &ui_submenu,
    .subitems = {&menu_cutoff_enabled, &menu_cutoff_value, 0}
};

static const MenuItem menu_cutoff_enabled = {
    .caption = "ENAB",
    .handler = &ui_select_item,
    .data = &settings.cutoff_enabled,
    .subitems = {&menu_on,  &menu_off,  0}
};

static const NumericEdit menu_cutoff_value_edit = {
    .var = &settings.cutoff_voltage,
    .min = 0, //mV
    .max = 30000, //mV
    .dot_offset = 3,
};

static const MenuItem menu_cutoff_value = {
    .caption = "CVAL",
    .handler = &ui_edit_value,
    .data = &menu_cutoff_value_edit,
    .value = LED_V
};

const MenuItem menu_active = {
    .caption = "ACT ",
    .handler = &ui_active,
};

const MenuItem menu_error = {
    .caption = "err ",
    .handler = &ui_error_handler,
};
