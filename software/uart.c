#include "uart.h"
#include "config.h"
#include "stm8s.h"

void setupUART()
{
	uint32_t Mant, Mant100;
	Mant = ((uint32_t)F_CPU / (BAUDR << 4));
	Mant100 = (((uint32_t)F_CPU * 100) / (BAUDR << 4));
#ifdef STM8S003
	UART1->BRR2  = (uint8_t)((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F);
	UART1->BRR2 |= (uint8_t)((Mant >> 4) & (uint8_t)0xF0);
	UART1->BRR1  = (uint8_t)Mant;
	UART1->CR2 = UART1_CR2_TEN | UART1_CR2_REN;
#else
	UART2->BRR2 = (uint8_t)(((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F) | ((Mant >> 4) & (uint8_t)0xF0));
	UART2->BRR1 = (uint8_t)Mant;
	UART2->CR2 = UART2_CR2_TEN | UART2_CR2_REN;
	UART2->CR2 |= (uint8_t)((uint8_t)1 << (uint8_t)((uint8_t)0x0205 & (uint8_t)0x0F)); //enable RX interrupt
#endif
}
