#include "settings.h"
#include "eeprom.h"
bool logging = 0;
sink_mode_t set_mode = MODE_CC;
uint16_t set_values[4]; // CC/CW/CR/CV
bool beeper_enabled = 0;
uint16_t cutoff_voltage = 270;
bool cutoff_active = 0;

void settings_init()
{
    set_mode = read8(MEM_MODE);
	set_values[MODE_CC] = read16(MEM_CC);
	set_values[MODE_CW] = read16(MEM_CW);
	set_values[MODE_CR] = read16(MEM_CR);
	set_values[MODE_CV] = read16(MEM_CV);
	beeper_enabled = read8(MEM_BEEP);
	cutoff_active = read8(MEM_CUTO);
	cutoff_voltage = read16(MEM_CUTV);
}
