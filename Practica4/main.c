#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "string.h"


static const char *TAG = "Practica 4";
#define SPI_MASTER_NUM       SPI3_HOST
#define SPI_MASTER_MISO_PIN  19
#define SPI_MASTER_MOSI_PIN  23
#define SPI_MASTER_SCLK_PIN  18
#define SPI_MASTER_CS_PIN    5

/**** CONSTANTES PRACTICA 4****/
#define WHO_AM_I        0x0F
#define OUT_X_H         0x29
#define OUT_X_L         0x28
#define OUT_Y_H         0x2B
#define OUT_Y_L         0x2A
#define OUT_Z_H         0x2D
#define OUT_Z_L         0x2C
#define LEN             8
#define ADDR            0x18
#define ODDR            0x01
#define MODE            0x01
#define LP_MODE         0x01
#define CONFG           (ODDR << 4 | MODE << 2| LP_MODE)

spi_device_handle_t spi;

void delay(uint32_t ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static esp_err_t spi_master_init(void){
    esp_err_t ret;
    spi_bus_config_t config ={
        .mosi_io_num = SPI_MASTER_MOSI_PIN,
        .miso_io_num = SPI_MASTER_MISO_PIN,
        .sclk_io_num = SPI_MASTER_SCLK_PIN,
        .quadwp_io_num=-1, .quadhd_io_num=-1
    };
    ret = spi_bus_initialize(SPI_MASTER_NUM, &config, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
 
    spi_device_interface_config_t devconfig = {
        .mode = 1,
        .spics_io_num = SPI_MASTER_CS_PIN,
        .queue_size = 1,
        .clock_speed_hz = 1.6,
        .pre_cb = NULL,
        .address_bits = 8
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_MASTER_NUM, &devconfig, &spi));
    return ret;
}

esp_err_t device_register_read(uint8_t reg_addr, uint8_t *data, size_t len){
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = reg_addr | 0x80;
    t.length = len;
    t.rx_buffer = data;
    esp_err_t ret = spi_device_transmit(spi, &t);
    ESP_ERROR_CHECK(ret);
    return ret;
}

esp_err_t device_register_write(uint8_t reg_addr, uint8_t data){
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = reg_addr & 0x7F;
    t.length = 8;
    t.tx_buffer = &data;
    ret = spi_device_transmit(spi, &t);
    ESP_ERROR_CHECK(ret);
    return ret;
}

void print_data(char* str, uint8_t addr){
    uint8_t data = 0;
    char test[40];
    if (device_register_read(addr, &data, LEN) == ESP_OK)
        sprintf(test,"%s \t= 0x%x",str, data);
    else
        sprintf(test,"ERROR %s \t= 0x%x",str,data);
    ESP_LOGI(TAG, "%s", test);
}

void app_main(void)
{
    ESP_ERROR_CHECK(spi_master_init());
    ESP_LOGI(TAG, "SPI initialized successfully");
    uint8_t data = CONFG;
    device_register_write(0x20, data);
    while (1){
        print_data("WHO_I_AM", WHO_AM_I);
        print_data("OUT_X_H", OUT_X_H);
        print_data("OUT_X_L", OUT_X_L);
        print_data("OUT_Y_H", OUT_Y_H);
        print_data("OUT_Y_L", OUT_Y_L);
        print_data("OUT_Z_H", OUT_Z_H);
        print_data("OUT_Z_L", OUT_Z_L);
        ESP_LOGI(TAG, "\n");
        delay(3000);
    }
    ESP_ERROR_CHECK(spi_bus_free(SPI_MASTER_NUM));
    ESP_LOGI(TAG, "SPI unitialized successfully");
}
