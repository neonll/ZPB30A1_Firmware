#include "settings.h"
#include "eeprom.h"
bool logging = 0;
sink_mode_t set_mode = MODE_CC;
uint16_t set_values[4]; // CC/CW/CR/CV
bool beeper_enabled = 0;
uint16_t cutoff_voltage = 270;
bool cutoff_active = 0;

#define MEM_CHECK 0x00
#define MEM_MODE  0x01
#define MEM_BEEP  0x02
#define MEM_CUTO  0x03
#define MEM_VALUES 0x10 /*8 bytes */

#define MEM_CUTV  0x50
//                0x51
#define MEM_TOUT  0x52
//                0x53

void settings_init()
{
    //TODO: Check if eeprom contents are valid and use default values otherwise
    set_mode = (sink_mode_t)eeprom_read8(MEM_MODE);
    uint8_t i;
    for (i=0; i<NUM_MODES; i++) {
	    set_values[i] = eeprom_read16(MEM_VALUES+2*i);
    }
	beeper_enabled = eeprom_read8(MEM_BEEP);
	cutoff_active = eeprom_read8(MEM_CUTO);
	cutoff_voltage = eeprom_read16(MEM_CUTV);
}

void settings_update()
{
    //eeprom_write_XX() only writes changed data
    eeprom_write8(MEM_MODE, set_mode);
    uint8_t i;
    for (i=0; i<NUM_MODES; i++) {
        eeprom_write16(MEM_VALUES+2*i, set_values[i]);
    }
    eeprom_write8(MEM_BEEP, beeper_enabled);
    eeprom_write8(MEM_CUTO, cutoff_active);
    eeprom_write16(MEM_CUTV, cutoff_voltage);
}
