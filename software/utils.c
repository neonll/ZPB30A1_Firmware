#include "utils.h"
#include "timer.h"
#include "config.h"

void delay10ms(uint32_t d)
{
	uint32_t start = systick;
	while(systick / (F_SYSTICK/100) < start + d);
}
