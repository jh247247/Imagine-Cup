#ifndef USART_H
#define USART_H
#include "stm32f10x_usart.h"

#define USART_HAS_DATA(x) (USART_GetITStatus(x, USART_IT_RXNE) != RESET)

#define USART_BUF_LEN 50
#define USART_AMOUNT 2

extern volatile char g_usart_rx[USART_AMOUNT][USART_BUF_LEN];
extern volatile int g_usart_rx_index[USART_AMOUNT];

void USART12_Init(void);
void USART1_PutChar(char ch);
void USART1_PutString(char * str);
void USART2_PutChar(char ch);
void USART2_PutString(char * str);
unsigned char USART_waitForString(USART_TypeDef* USARTx, char* ref,
				  int timeout);
void USART_setMatch(int usart, char* str);
void USART_resetMatch(int usart);
void USART_resetRXBuffer(int usart);
int USART_checkMatch(int usart);

#endif /* USART_H */
