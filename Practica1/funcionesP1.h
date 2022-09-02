#include "driver/uart.h"
// UART setup
void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop, uint8_t txPin, uint8_t rxPin);
// Receive
bool uartKbhit(uart_port_t uart_num);
void uartPutchar(uart_port_t uart_num, char c);
void uartPuts(uart_port_t uart_num, char *str);
char uartGetchar(uart_port_t uart_num);
void uartGets(uart_port_t uart_num, char *str);
// Escape sequences
void uartClrScr(uart_port_t uart_num);
void uartGotoxy(uart_port_t uart_num, uint8_t x, uint8_t y);
//Timer for sender
void clearTimer0();
void setupTimer0();
//vTaskDelay
void delayMs(uint16_t ms);