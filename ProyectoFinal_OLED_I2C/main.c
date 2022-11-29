#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "esp_eth.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "time.h"
#include "string.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "sdkconfig.h"

#include "oled_utils.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "cJSON.h"
#include "communication_protocol.h"

//CAMBIAR HTTPD_MAX_REQ_HDR_LEN DE 512 A 1024
#define EXAMPLE_ESP_WIFI_SSID      "temp"
#define EXAMPLE_ESP_WIFI_PASS      "123456789"
#define EXAMPLE_ESP_MAXIMUM_RETRY       5

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    switch (event_id){
        case WIFI_EVENT_STA_START:
            printf("WiFi connecting ... \n");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            printf("WiFi connected ... \n");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            printf("WiFi lost connection ... \n");
            break;
        case IP_EVENT_STA_GOT_IP:
            printf("WiFi got IP ... \n\n");
            break;
        default:
            break;
    }
}
void wifi_connection(){
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation                   s1.1
    esp_event_loop_create_default();     // event loop                          s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station                        s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); //                                         s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        }
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_connect();
}

void delayMs(uint32_t ms){
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt){
    switch (evt->event_id){
        char aux[500];
        case HTTP_EVENT_ON_DATA:
            sprintf(aux, "%.*s\n", evt->data_len, (char *)evt->data);
            cJSON *root = cJSON_Parse(aux);
            char value[100];
            sprintf(value, "%s", cJSON_GetObjectItem(root, "node")->valuestring);
            comPackage = packageDecode(value);
            if(comPackage.crc32 != -1){
                ESP_LOGI("GET", ": valor recibido: \n");
                //decodeValue(comPackage.data);
                char* pend;
                float f1 = strtof((char*)comPackage.data, &pend);
                float f2 = strtof(pend, NULL);
                char f1str[50];
                char f2str[50];
                sprintf(f1str, "Temp: %.3f", f1);
                sprintf(f2str, "Humedad: %.3f", f2);
                i2c_oled_master_init();
                oled_init();
                oled_clear();
                oled_write_str(f1str, 0);
                oled_write_str(f2str, 1);
                ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
            }else{
            
                i2c_oled_master_init();
                oled_init();
                oled_clear();
                oled_write_str("Data error", 0);
                oled_write_str("CRC Check is different, error code 1597", 1);
                ESP_ERROR_CHECK(i2c_driver_delete(I2C_MASTER_NUM));
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}
static void sensors_rest_get(){
    esp_http_client_config_t config_get = {
        .url = "http://192.168.4.1/api/v1/temp/raw",
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL,
        .event_handler = client_event_get_handler
    };
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}




void app_main(void){
    /*
    //Samples de las funciones de la pantalla jaja
    oled_clear();
    oled_fill_yellow();
    oled_fill_blue();
    oled_clear_yellow();
    oled_clear_blue();
    oled_write_str("User: Holi", 0);
    oled_fill();
    oled_send_data(&enfriar[0],1024);
    delay(2000);
	}*/
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");
    while(1){
        sensors_rest_get();
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}