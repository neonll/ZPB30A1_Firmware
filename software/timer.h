#ifndef _TIMER_H_
#define _TIMER_H_
#include <stdint.h>
void systick_init();
extern volatile uint32_t systick;

#endif
