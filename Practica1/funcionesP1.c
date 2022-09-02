#include "driver/uart.h"
#include "definesP1.h"
#include "funcionesP1.h"
void uartInit(uart_port_t uart_num, uint32_t baudrate, uint8_t size, uint8_t parity, uint8_t stop, uint8_t txPin, uint8_t rxPin)
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
    ESP_ERROR_CHECK(uart_set_pin(uart_num, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

}
bool uartKbhit(uart_port_t uart_num){
    uint8_t length;
    uart_get_buffered_data_len(uart_num, (size_t*)&length);
    return (length > 0);
}
void uartPutchar(uart_port_t uart_num, char c){
    uart_write_bytes(uart_num, &c, sizeof(c));
}
void uartPuts(uart_port_t uart_num, char *str){
    int i = 0;

    while(str[i] != '\0'){
        uartPutchar(uart_num, str[i]);
        i++;
    }
}
char uartGetchar(uart_port_t uart_num)
{
    char c;
    // Wait for a received byte
    while(!uartKbhit(uart_num))
    {
        delayMs(10);
    }
    // read byte, no wait
    uart_read_bytes(uart_num, &c, sizeof(c), 0);

    return c;
}
void uartGets(uart_port_t uart_num, char *str){
    int i = 0;
    while(1){
        char a = uartGetchar(uart_num);
        if(a != 13){
            if(a == 127 || a == 8){
                uartPuts(uart_num, "\e[1D");
                uartPuts(uart_num, "\e[0K");
                i--;
                if(i < 0) i = 0;
                str[i] = '\0';
                uart_flush(uart_num);
                uart_flush_input(uart_num);
            }
            else{
                uart_write_bytes(uart_num, &a, sizeof(a));
                str[i] = a;
                i++;
            }

        } 
        else break;
    }
    str[i] = '\0';
}
void uartClrScr(uart_port_t uart_num)
{
    // Uso "const" para sugerir que el contenido del arreglo lo coloque en Flash y no en RAM
    const char caClearScr[] = "\e[2J";
    uart_write_bytes(uart_num, caClearScr, sizeof(caClearScr));
}
void uartGotoxy(uart_port_t uart_num, uint8_t x, uint8_t y){
    char caGotoxy[11];
    sprintf(caGotoxy, "\e[%d;%dH", x, y);
    uartPuts(uart_num, caGotoxy);
}
void delayMs(uint16_t ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}
void clearTimer0(){
    TIMG0_TOLOADLO_REG = 0;
    TIMG0_TOLOADHI_REG = 0;
    TIMG0_TOLOAD_REG = 0;
    TIMG0_CONFIG_REG = 0;
}
void setupTimer0(){
    TIMG0_CONFIG_REG |=    (3<<29)      | (1<<10) | ((uint32_t)800 << 13);
    TIMG0_TOALARMLO_REG = 300000;//A los 3 seg
    TIMG0_TOALARMHI_REG = 0;
    TIMG0_CONFIG_REG |= (1 << 31); 
}



