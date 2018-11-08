#include "uart.h"
#include "config.h"
#include "inc/stm8s_uart2.h"
#include "inc/stm8s_itc.h"
#include <stdio.h>

void uart_init()
{
	uint32_t Mant, Mant100;
	Mant = ((uint32_t)F_CPU / (BAUDR << 4));
	Mant100 = (((uint32_t)F_CPU * 100) / (BAUDR << 4));
	UART2->BRR2 = (uint8_t)(((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F) | ((Mant >> 4) & (uint8_t)0xF0));
	UART2->BRR1 = (uint8_t)Mant;
	UART2->CR2 = UART2_CR2_TEN | UART2_CR2_REN | UART2_CR2_RIEN;
}

int putchar(int c)
{
	UART2->DR = (char) c;
	while (!(UART2->SR & (uint8_t)UART2_FLAG_TXE));
	return c;
}


void uart_rx_irq() __interrupt(ITC_IRQ_UART2_RX)
{
	char tmp = UART2->DR;
}
