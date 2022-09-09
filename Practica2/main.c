/*
    Practica 2: UART - Protocolo completo
    Adame Arroyo Adrian Eduardo  1272202
    Landa Luna Edgar Miguel  1263337
    Zavala Roman Irvin Eduardo 1270771

    uartInit(0,115200,8,0,2, 1, 3);
    uartInit(1,115200,8,0,1, 10, 9);
    uartInit(2,115200,8,0,1, 17, 16);

    uC Sender | uC RECEIVER
    UART1_TX -> UART2_RX
    UART1_RX <- UART2_TX
     CORE 0      CORE 1

    0x10: Timestamp
    0x11: Estado led
    0x12: Temperatura sensor imaginario
    0x13: Invertir estado led
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "time.h"

#include "functions.h"
#include "constantValues.h"

#define DEBUG 1

#define SENDER 1
#define RECEIVER 0

void Sender( void * parameter ){
    UART_Package pkg;

    uint8_t *received_data = (uint8_t *) malloc(READ_BUF_SIZE);
    char string[40];
    char *data_to_send;
    while(1){ 
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
            pkg = createPackage(0x5A, (uint8_t)strtol(string, NULL, 0), 0x0, NULL, 0xB2);
            data_to_send = (char*) malloc(sizeof(char));

            uartStruct_encode(data_to_send, pkg);

            uartPuts(1, data_to_send);
            free(data_to_send);
            
            setupTimer0(); //Inicio timer 3 seg
            while(1){
                int len = uart_read_bytes(1, received_data, (READ_BUF_SIZE * 2), 20 / portTICK_RATE_MS);
                if(len){    
                    UART_Package pkg_received = uartStruct_decode((char *)received_data);

                    #if DEBUG == 1
                        uartClrScr(0);
                        uartGotoxy(0,2,2);
                        ESP_LOGI("Estructura recibida", "%c\n\n", (const char)' ');
                        printStruct(pkg_received);
                        ESP_LOGI("Esto es una prueba, recordar cambiar DEBUG a 0\nPresiona enter para continuar", "%c\n\n", (const char)' ');
                        uartGetchar(0);
                    #endif
                    
                    uartClrScr(0);
                    uartGotoxy(0,2,2);
                    if(pkg_received.command == 0x10) uartPuts(0, "Timestamp");
                    if(pkg_received.command == 0x11) uartPuts(0, "Estado led");
                    if(pkg_received.command == 0x12) uartPuts(0, "Temperatura en C");
                    if(pkg_received.command == 0x13) uartPuts(0, "Comando 0x13");
                    uartGotoxy(0,4,2);
                    uartPuts(0, (char*) pkg_received.data);
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
        delayMs(5); //Necesario para el watchdog
    }
}

void Receiver( void * parameter ){
    uint8_t *received_data = (uint8_t *) malloc(READ_BUF_SIZE);
    char string[100]; //Tamano cambiado para soportar el tamano del protocolo
    char *data_to_send;
    UART_Package pkg;
    while(1){ 
        srand(time(NULL));
        GPIO_OUT_REG |= (1 << 2);
        int len = uart_read_bytes(2, received_data, (READ_BUF_SIZE * 2), 20 / portTICK_RATE_MS);
        if (len) {
            pkg = uartStruct_decode((char *)received_data);

            uint8_t comand = pkg.command;

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
                    strcpy(string, "Led Invertido");
                    break;
                default:
                    break;
            }
            pkg = createPackage(0x5A, comand, strlen((const char *)string), (uint8_t*)string, 0xB2);

            data_to_send = (char*) malloc(sizeof(char));
            uartStruct_encode(data_to_send, pkg);
            
            uartPuts(2, data_to_send);
            free(data_to_send);
        }
        delayMs(5); //Necesario para el watchdog
    }
}

void app_main(void)
{   
    uartInit(0,115200,8,0,1, 1, 3);
    uartInit(1,115200,8,0,1, 19, 18);
    uartInit(2,115200,8,0,1, 17, 16);

    uart_flush(0); uart_flush(1);

    #if SENDER == 1 
        xTaskCreatePinnedToCore(
            Sender,
            "Sender",
            10000,
            NULL,
            1,
            NULL,
            0
        );
        delayMs(5);
    #endif
    #if RECEIVER == 1
        xTaskCreatePinnedToCore(
        Receiver,
        "receiver",
        10000,
        NULL,
        1,
        NULL,
        1
        );
        delayMs(5);
    #endif
}