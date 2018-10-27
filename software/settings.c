#include "settings.h"
#include "eeprom.h"
settings_t settings;

/* Note: The checksum is placed after the data so when the settings size grows
   The checksum automatically becomes invalid. */

static uint8_t settings_calc_checksum(uint8_t *data, uint16_t size)
{
    uint16_t i;
    uint8_t checksum = 0x55;
    for (i = 0; i < size; i++)
    {
        checksum ^= *data++;
    }
    return checksum;
}

void settings_init()
{
    uint16_t addr;
    uint8_t *data = (uint8_t*)(&settings);
    for (addr = 0; addr < sizeof(settings); addr++)
    {
        data[addr] = eeprom_read8(addr);
    }
    uint8_t checksum = eeprom_read8(sizeof(settings));
    if (checksum != settings_calc_checksum(data, sizeof(settings))) {
        // Invalid checksum => initialize default values
        settings.mode = MODE_CC;
        settings.setpoints[MODE_CC] = 0;
        settings.setpoints[MODE_CW] = 0;
        settings.setpoints[MODE_CR] = 0;
        settings.setpoints[MODE_CV] = 0;
        settings.beeper_enabled = 1;
        settings.cutoff_enabled = 0;
        settings.cutoff_voltage = 123;
    }
}

void settings_update()
{
    uint16_t addr;
    uint8_t *data = (uint8_t*)(&settings);
    for (addr = 0; addr < sizeof(settings); addr++)
    {
        eeprom_write8(addr, data[addr]);
    }
    uint8_t checksum = settings_calc_checksum(data, sizeof(settings));
    eeprom_write8(sizeof(settings), checksum);
}
