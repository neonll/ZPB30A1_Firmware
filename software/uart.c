#include "uart.h"
#include "config.h"
#include "inc/stm8s_uart2.h"
#include "inc/stm8s_itc.h"
#include <stdio.h>
#include "adc.h"
#include "load.h"

void uart_init()
{
	uint16_t uart_div =  (F_CPU + BAUDR/2) / BAUDR;
	UART2->BRR2 = (uart_div >> 12) | (uart_div & 0x0f);
	UART2->BRR1 = (uart_div >> 4);
	UART2->CR2 = UART2_CR2_TEN | UART2_CR2_REN | UART2_CR2_RIEN;
}

int putchar(int c)
{
	UART2->DR = (char) c;
	while (!(UART2->SR & (uint8_t)UART2_FLAG_TXE));
	return c;
}

void uart_timer()
{
	static uint16_t timer = 0;
	timer++;
	if (timer == F_SYSTICK/F_LOG) {
		timer = 0;
		printf("T: %3u Vi: %5u Vl: %5u Vs: %5u C: %5u mWs: %5lu mAs: %5lu\r\n", temperature, v_12V, v_load, v_sense, actual_current_setpoint, mWatt_seconds, mAmpere_seconds);
	}
}


void uart_rx_irq() __interrupt(ITC_IRQ_UART2_RX)
{
	char tmp = UART2->DR;
}
