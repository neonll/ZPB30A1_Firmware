#include "load.h"
bool cutoff_active = 0;
uint16_t cutoff_voltage = 270;
uint16_t voltage = 0;	// 0,01V
uint16_t set_current = 0;	// 0,001A
volatile uint32_t mAmpere_seconds = 0;	//mAs
volatile uint32_t mWatt_seconds = 0;	//mWs
bool running = 0;
