#ifndef _EEPROM_H_
#define _EEPROM_H_
#include <stdint.h>
#define MEM_CHECK 0x00
#define MEM_MODE  0x01
#define MEM_BEEP  0x02
#define MEM_CUTO  0x03
#define MEM_CC    0x10
//                0x11
#define MEM_CW    0x20
//                0x21
#define MEM_CR    0x30
//                0x31
#define MEM_CV    0x40
//                0x41
#define MEM_CUTV  0x50
//                0x51
#define MEM_TOUT  0x52
//                0x53

uint8_t read8(uint16_t address);
uint16_t read16(uint16_t address);
void write8(uint16_t address, uint8_t data);
void write16(uint16_t address, uint16_t data);
#endif
