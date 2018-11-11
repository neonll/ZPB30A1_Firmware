#ifndef _LOAD_H_
#define _LOAD_H_
#include <stdbool.h>
#include <stdint.h>

//NOTE: Keep this enum in sync with the messages in ui_error_handler()!
typedef enum {
	ERROR_NONE,
	ERROR_POLARITY, // input polarity reversed
	ERROR_OVERVOLTAGE, // input voltage to high
	ERROR_OVERLOAD,
	ERROR_OVERTEMPERATURE, // heat sink temperature to high
	ERROR_POWER_SUPPLY, // supply voltage to low
	ERROR_TIMER_OVERFLOW, // timer tick lost
	ERROR_INTERNAL, // other internal error
} error_t;

extern error_t error;
extern uint16_t  voltage; //0,01V
extern uint16_t set_current; //mA
extern volatile uint32_t mAmpere_seconds;	//mAs
extern volatile uint32_t mWatt_seconds;	//mWs
extern bool load_active;

void load_init();
void load_timer();
void load_enable();
void load_disable();


void calcPWM(void);
void getVoltage(void);

#endif
