#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

static const char *TAG = "Practica 3";

#define I2C_MASTER_SCL_IO           22         /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21         /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

/**** CONSTANTES PRACTICA 3****/
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

void delay(uint32_t ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static esp_err_t device_register_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDR << 1), 0);
    i2c_master_write_byte(cmd, reg_addr, 0);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDR << 1) | 1, 0);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t device_register_write(uint8_t reg_addr, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ADDR << 1), 0);
    i2c_master_write_byte(cmd, reg_addr, 0);
    i2c_master_write_byte(cmd, data, 0);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

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

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    uint8_t data = CONFG;
    device_register_write(0x20, data);
    
    while (1)
    {
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

    ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
    ESP_LOGI(TAG, "I2C unitialized successfully");
}
