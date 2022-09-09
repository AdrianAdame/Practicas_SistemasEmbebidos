#include "driver/uart.h"

//Struct for create a UART Package
typedef struct UART_Package{
    uint8_t header, command, length, end;
    uint8_t *data;
    uint32_t crc32;
} UART_Package;

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

//Dynamic array allocation
void array_append(char *arr, int size, char value);

//Calculate Cyclic Redundancy Check 32b
uint32_t crc32b(char *message);

//UART Package functions
UART_Package createPackage(uint8_t header, uint8_t command, uint8_t length, uint8_t *data, uint8_t end);
UART_Package uartStruct_decode(char *str);
void uartStruct_encode(char *str, UART_Package pkg);
void printStruct(UART_Package pkg);