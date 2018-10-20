#include "eeprom.h"
#include "stm8s_flash.h"
#define _MEM_(mem_addr) (*(volatile uint8_t *)(mem_addr))

//TODO: Check if eeprom contents are valid and use default values otherwise

uint8_t read8(uint16_t address) {
	return _MEM_(address + FLASH_DATA_START_PHYSICAL_ADDRESS);
}

uint16_t read16(uint16_t address) {
	return (read8(address) << 8) | read8(address + 1);
}

void write8(uint16_t address, uint8_t data) {
	_MEM_(address + FLASH_DATA_START_PHYSICAL_ADDRESS) = data;
}

void write16(uint16_t address, uint16_t data) {
	write8(address, data >> 8);
	write8(address + 1, data & 0xFF);
}
