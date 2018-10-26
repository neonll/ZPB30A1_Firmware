#include "uart.h"
#include "config.h"
#include "stm8s_uart2.h"
#include "todo.h"
#include <stdio.h>

void uart_init()
{
	uint32_t Mant, Mant100;
	Mant = ((uint32_t)F_CPU / (BAUDR << 4));
	Mant100 = (((uint32_t)F_CPU * 100) / (BAUDR << 4));
	UART2->BRR2 = (uint8_t)(((uint8_t)(((Mant100 - (Mant * 100)) << 4) / 100) & (uint8_t)0x0F) | ((Mant >> 4) & (uint8_t)0xF0));
	UART2->BRR1 = (uint8_t)Mant;
	UART2->CR2 = UART2_CR2_TEN | UART2_CR2_REN;
	UART2->CR2 |= (uint8_t)((uint8_t)1 << (uint8_t)((uint8_t)0x0205 & (uint8_t)0x0F)); //enable RX interrupt
}

int putchar(int c)
{
	UART2->DR = (char) c;
	while (!(UART2->SR & (uint8_t)UART2_FLAG_TXE));
	return c;
}

int getchar(void)
{
	return UART2->DR;
}

//TODO: Rewrite this function. printf in IRQ context is never a good idea.
void UART2_RX_IRQHandler() __interrupt(21)
{
	char tmp = UART2->DR;
	if(tmp == 'S'){	//start command from LogView
		printf("$N$;Electronic Load\r\n");
		printf("$C$;Current [A,I];Voltage [V,U];Ampere hour[Ah];Watt hour[Wh];Temperature[°C,T]\r\n");
		printf("$F$;0.001;0.01;0.000000277778;0.000000277778;0,1\r\n");	//convert units for logview
		logging = 1;
	}
	else if(tmp == 'E') {
		printf("End of logging");
		logging = 0;
	}
}
