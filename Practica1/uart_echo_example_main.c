/*
    Practica 1: UART
    Adame Arroyo Adrian Eduardo  1272202
    Landa Luna Edgar Miguel  1263337
    Zavala Roman Irvin Eduardo 1270771

    uartInit(0,115200,8,0,2, 1, 3);
    uartInit(2,115200,8,0,1, 17, 16);

    uC Sender | uC RECEIVER
    UART2_TX -> UART2_RX
    UART2_RX <- UART2_TX

    La comunicacion se da entre UART2 de los 2 esp

    0x10: Timestamp
    0x11: Estado led
    0x12: Temperatura sensor imaginario
    0x13: Invertir estado led
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "funcionesP1.h"
#include "definesP1.h"

#define SENDER      0
#define RECEIVER    1
#define MODE        RECEIVER  

void app_main(void)
{   
    uartInit(0,115200,8,0,1, 1, 3);
    uartInit(2,115200,8,0,1, 17, 16);
    uint8_t *received_data = (uint8_t *) malloc(READ_BUF_SIZE);
    char string[20];
    uart_flush(0); uart_flush(1);

    while(1){ 
        #if MODE == RECEIVER
            GPIO_OUT_REG |= (1 << 2);
            int len = uart_read_bytes(2, received_data, (READ_BUF_SIZE - 1), 20 / portTICK_RATE_MS);
            if (len) {
                received_data[len] = '\0';
                uint8_t comand = strtol((char *)received_data , NULL , 0);
                switch (comand)
                {
                    case 0x10:
                        itoa(xTaskGetTickCount(),string,10);
                        break;
                    case 0x11:
                        itoa((GPIO_ENABLE_REG >> 2) & 1, string, 10);
                        break;
                    case 0x12:
                        itoa(rand()%30, string, 10);
                        break;
                    case 0x13:
                        if((GPIO_ENABLE_REG >> 2) & 1) GPIO_ENABLE_REG &= ~(1 << 2);
                        else GPIO_ENABLE_REG |= 1 << 2;
                        strcpy(string, "Led invertido");
                        break;
                    default:
                        break;
                }
                uartPuts(2, string);
            }
        #elif MODE == SENDER
            uart_flush(0); uart_flush(1);
            uartClrScr(0);
            uartGotoxy(0,2,2);
            uartPuts(0, "Introduce un comando: ");
            uartGets(0, string);
            if(    !strcmp(string,"0x10") 
                || !strcmp(string,"0x11")
                || !strcmp(string,"0x12") 
                || !strcmp(string,"0x13"))
            {
                uartPuts(2, string);  
                setupTimer0(); //Inicio timer 3 seg
                while(1){
                    int len = uart_read_bytes(2, received_data, READ_BUF_SIZE, 20 / portTICK_RATE_MS);
                    if(len){   
                        received_data[len] = '\0';
                        uartClrScr(0);
                        uartGotoxy(0,2,2);
                        if(!strcmp(string,"0x10")) uartPuts(0, "Timestamp");
                        if(!strcmp(string,"0x11")) uartPuts(0, "Estado led");
                        if(!strcmp(string,"0x12")) uartPuts(0, "Temperatura en C");
                        if(!strcmp(string,"0x13")) uartPuts(0, "Comando 0x13");
                        uartGotoxy(0,4,2);
                        uartPuts(0, (char*) received_data);
                        break;
                    }            
                    if((TIMG0_CONFIG_REG & (1<<10)) == 0){ //Fin timer
                        uartClrScr(0);
                        uartGotoxy(0,2,2);
                        ESP_LOGE("RECEIVE ERROR", "%s\n", "No data received\n\tPress a key to continue");
                        break;
                    }
                }
                clearTimer0(); //Se limpia el timer
                string[0] = '\0'; 
            }
            else{
                uartClrScr(0);
                uartGotoxy(0,2,2);
                ESP_LOGE("USER ERROR", "%s\n", "Invalid command...\n\tPress a key to continue");
            }
            uartGetchar(0);
        #endif
            delayMs(5); //Necesario para el watchdog
    }
}