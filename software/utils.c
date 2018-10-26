#include "utils.h"
#include "timer.h"
#include "config.h"

//TODO: Replace by real delay function. The compiler can optimize this one away
void delay(uint32_t d)
{
	while (d != 0) {
		d--;
	}
}

//TODO: Either the function name or "millis" is misleading
void delay10ms(uint32_t d)
{
	uint32_t start = systick;
	while(systick / (F_SYSTICK/100) < start + d);
}
