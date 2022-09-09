#include "esp_log.h"
#include "stdlib.h"
#include "string.h"
#include "functions.h"
#include "constantValues.h"

/**
 * @brief Configure uart peripheral for the ESP32 via structs
 * 
 * @param uart_num Number of uart to use (For ESP32 - 0, 1, 2)
 * @param baudrate Velocity of synchronization, sends all the bits with this baudrate
 * @param size Size of data payload
 * @param parity Configure if the protocol to use will use parity - WARNING: This will limit the data payload
 * @param stop How many stop bits will be (For ESP32 - 1, 1.5, 2)
 * @param txPin GPIO defined for the UART TX
 * @param rxPin GPIO defined for the UART RX
 */
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

/**
 * @brief Check if there is data coming from the UART
 * 
 * @param uart_num uart number to check
 * @return true if there is data in the uart
 * @return false if there's not data in the uart
 */
bool uartKbhit(uart_port_t uart_num){
    uint8_t length;
    uart_get_buffered_data_len(uart_num, (size_t*)&length);
    return (length > 0);
}

/**
 * @brief Write a character to the uart
 * 
 * @param uart_num Number of uart to send the character
 * @param c Character to send
 */
void uartPutchar(uart_port_t uart_num, char c){
    uart_write_bytes(uart_num, &c, sizeof(c));
}

/**
 * @brief Write a char array to the uart
 * 
 * @param uart_num Number of uart to send the character
 * @param str String to send
 */
void uartPuts(uart_port_t uart_num, char *str){
    int i = 0;

    while(str[i] != '\0'){
        uartPutchar(uart_num, str[i]);
        i++;
    }
}

/**
 * @brief 
 * 
 * @param uart_num 
 * @return char 
 */
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

/**
 * @brief 
 * 
 * @param uart_num 
 * @param str 
 */
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

/**
 * @brief 
 * 
 * @param uart_num 
 */
void uartClrScr(uart_port_t uart_num)
{
    // Uso "const" para sugerir que el contenido del arreglo lo coloque en Flash y no en RAM
    const char caClearScr[] = "\e[2J";
    uart_write_bytes(uart_num, caClearScr, sizeof(caClearScr));
}

/**
 * @brief 
 * 
 * @param uart_num 
 * @param x 
 * @param y 
 */
void uartGotoxy(uart_port_t uart_num, uint8_t x, uint8_t y){
    char caGotoxy[11];
    sprintf(caGotoxy, "\e[%d;%dH", x, y);
    uartPuts(uart_num, caGotoxy);
}

/**
 * @brief 
 * 
 */
void clearTimer0(){
    TIMG0_TOLOADLO_REG = 0;
    TIMG0_TOLOADHI_REG = 0;
    TIMG0_TOLOAD_REG = 0;
    TIMG0_CONFIG_REG = 0;
}

/**
 * @brief 
 * 
 */
void setupTimer0(){
    TIMG0_CONFIG_REG |=    (3<<29)      | (1<<10) | ((uint32_t)800 << 13);
    TIMG0_TOALARMLO_REG = 300000;//A los 3 seg
    TIMG0_TOALARMHI_REG = 0;
    TIMG0_CONFIG_REG |= (1 << 31); 
}

/**
 * @brief 
 * 
 * @param ms 
 */
void delayMs(uint16_t ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}


/**
 * @brief 
 * 
 * @param arr 
 * @param size 
 * @param value 
 */
void array_append(char *arr, int size, char value){
    int new_size = size + 1;
    
    arr = (char*) realloc(arr, new_size * (int)sizeof(char));
    arr[new_size - 1] = value;
}


/**
 * @brief 
 * 
 * @param message 
 * @return uint32_t 
 */
uint32_t crc32b(char *message) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (message[i] != 0xB2) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}


/**
 * @brief Create a Package object
 * 
 * @param header 
 * @param command 
 * @param length 
 * @param data 
 * @param end 
 * @return UART_Package 
 */
UART_Package createPackage(uint8_t header, uint8_t command, uint8_t length, uint8_t *data, uint8_t end){
    UART_Package pkg;
    
    pkg = (UART_Package) {
        .header = header, 
        .command = command, 
        .length = length,
        .end = end,
        .crc32 = 0
    };
    if(length > 0){
        pkg.data = (uint8_t*) malloc(sizeof(uint8_t) * length);
        memcpy(pkg.data, data, length+1);
    }else{
        pkg.data = NULL;
    }

    return pkg;
}

/**
 * @brief 
 * 
 * @param str 
 * @return UART_Package 
 */
UART_Package uartStruct_decode(char *str){
    UART_Package pkg;
    int i = 0;
    uint8_t position = 0, auxVar = 0;
    uint32_t crc32 = 0;

    pkg.header  = ((uint8_t) str[position++]);
    pkg.command = ((uint8_t) str[position++]);
    pkg.length = (((uint8_t) str[position++]));

    if(pkg.length > 0){
        pkg.data = (uint8_t*) malloc(sizeof(uint8_t) * pkg.length + 1); 
        for(; i < pkg.length; i++){
            pkg.data[i] = str[position++];
        }
        pkg.data[i] = '\0';
    }else{
        pkg.data = NULL;
    }

    pkg.end = ((uint8_t) str[position]);
    
    for(int i = 0; i < 3; i++){
        auxVar = ((uint8_t) str[position++]);
        crc32 |= auxVar;
        crc32 <<= 8;
    }
    crc32 |= ((uint8_t) str[position]);
    
    pkg.crc32 = 
    ((crc32 >> 24) & 0xFF) | 
    ((crc32 << 8) & 0xFF0000) | 
    ((crc32 >> 8) & 0xFF00) | 
    ((crc32 << 24) & 0xFF000000);

    /**
     * Aqui se podria agregar una funcion para ver si el crc32 que viene es el mismo que se calcula con los datos
    **/

    return pkg;
}

/**
 * @brief 
 * 
 * @param str 
 * @param pkg 
 */
void uartStruct_encode(char *str, UART_Package pkg){
    int size = 0;
    uint32_t crc32 = 0;

    array_append(str, size++, (char) pkg.header);
    array_append(str, size++, (char) pkg.command);

    array_append(str, size++, (char) (pkg.length == 0x0 ? -0x1 : pkg.length));

    for(int i = 0; i < pkg.length; i++){
        array_append(str, size++, (char)pkg.data[i]);
    }

    array_append(str, size++, (char) pkg.end);

    pkg.crc32 = crc32b(str);

    crc32 = pkg.crc32;

    for(int i = 0 ; i < 4; i++){
        array_append(str, size++, (char) crc32);
        crc32 >>= 8;
    }
}

/**
 * @brief 
 * 
 * @param pkg 
 */
void printStruct(UART_Package pkg){
    uartPuts(0, "\n");
    ESP_LOGI("Header", "0x%01X\n", pkg.header);
    ESP_LOGI("Command", "0x%01X\n", pkg.command);
    ESP_LOGI("Length", "0x%01X\n", pkg.length);
    if(pkg.length > 0)
        ESP_LOGI("*data", "%s\n", pkg.data); 
    else
        ESP_LOGI("*data", "NULL\n"); 
    ESP_LOGI("End", "0x%01X\n", pkg.end);
    ESP_LOGI("CRC32", "0x%08X\n", pkg.crc32);
}


