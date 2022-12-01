#include "myUart.h"

void delayMs(uint16_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

bool uartKbhit(uart_port_t uart_num)
{
    uint8_t length;
    uart_get_buffered_data_len(uart_num, (size_t*)&length);
    return (length > 0);
}
 
void uartPutchar(uart_port_t uart_num, char c)
{
    uart_write_bytes(uart_num, &c, sizeof(c));
}
 
char uartGetchar(uart_port_t uart_num)
{
    char c;
    // Wait for a received byte
    while(!uartKbhit(uart_num))
    {
        delayMs(10);
    }
    uart_read_bytes(uart_num, &c, sizeof(c), 0);
    return c;
}

void uartPuts(uart_port_t uart_num, char *str) {
    uint8_t i;
    for ( i = 0; str[i] != '\0'; i++)
    {
        uartPutchar(uart_num,str[i]);
    }  
}
 
void uartGets(uart_port_t uart_num, char *str) {
    uint8_t i = 0;
    char c;
    while (1)
    {
        c = uartGetchar(uart_num);
        if (c != 13)
        {
            if (c == 8) {
                if (i != 0)
                {
                    i--;
                    str[i] = '\0';
                    uartPuts(uart_num, "\e[1D");
                    uartPuts(uart_num, "\e[0K");
                }
 
            } else {
                uartPutchar(uart_num, c);
                str[i] = c;
                i++;
            }
        } else {
            str[i] = '\0';
            break;
        }
    }
    str[i]=0;
}
