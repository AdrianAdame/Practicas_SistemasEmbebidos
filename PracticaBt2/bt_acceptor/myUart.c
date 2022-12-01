#include "myUart.h"

#define UART_NUM                (0)

#define UARTS_BAUD_RATE         (115200)
#define TASK_STACK_SIZE         (1048)
#define READ_BUF_SIZE           (1024)

void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop)
{
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = size-5,
        .parity    = parity,
        .stop_bits = stop,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    ESP_ERROR_CHECK(uart_driver_install(uart_num, READ_BUF_SIZE, READ_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    if(uart_num == 0)
        ESP_ERROR_CHECK(uart_set_pin(uart_num, 1, 3,
                                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    if(uart_num == 1)
        ESP_ERROR_CHECK(uart_set_pin(uart_num, 10, 9,
                                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    if(uart_num == 2)
        ESP_ERROR_CHECK(uart_set_pin(uart_num, 17, 16,
                                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}
 
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