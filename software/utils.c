#include "utils.h"
#include "timer.h"

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
	uint32_t start = tenmillis;
	while(tenmillis < start + d);
}
