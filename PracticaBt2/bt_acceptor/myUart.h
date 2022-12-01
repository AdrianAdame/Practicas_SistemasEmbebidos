#ifndef MY_UART_H
#define MY_UART_H

#include "driver/uart.h"

void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop);

// Send
void uartPuts(uart_port_t uart_num, char *str);
void uartPutchar(uart_port_t uart_num, char data);

// Receive
bool uartKbhit(uart_port_t uart_num);
char uartGetchar(uart_port_t uart_num );
void uartGets(uart_port_t uart_num, char *str);

// Escape sequences
void uartClrScr( uart_port_t uart_num );
void uartSetColor(uart_port_t uart_num, uint8_t color);
void uartGotoxy(uart_port_t uart_num, uint8_t x, uint8_t y);

#define YELLOW  3 // Fixme 
#define GREEN   2 // Fixme 
#define BLUE    4 // Fixme 
bool compara_cadena(char * str, char *num);

// Utils
void myItoa(uint16_t number, char* str, uint8_t base) ;
uint16_t myAtoi(char *str);

#endif