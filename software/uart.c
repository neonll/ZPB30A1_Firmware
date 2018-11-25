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
	if (timer == F_SYSTICK/(F_LOG*7)) {
		timer = 0;
		static uint8_t cnt = 0;
		cnt++;
		if (cnt == 0) {
			printf("T %3u ", temperature);
		} else if (cnt == 1) {
			printf("Vi %5u ", v_12V);
		} else if (cnt == 2) {
			printf("Vl %5u ", v_load);
		} else if (cnt == 3) {
			printf("Vs %5u ", v_sense);
		} else if (cnt == 4) {
			printf("I %5u ", actual_current_setpoint);
		} else if (cnt == 5) {
			printf("mWs %10u ", mWatt_seconds);
		} else if (cnt == 6) {
			printf("mAs %10u ", mAmpere_seconds);
		} else {
			printf("\r\n");
			cnt = 0;
		}
	}
}


void uart_rx_irq() __interrupt(ITC_IRQ_UART2_RX)
{
	char tmp = UART2->DR;
}
