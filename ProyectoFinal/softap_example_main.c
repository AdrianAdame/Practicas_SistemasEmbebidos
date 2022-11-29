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
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "sdkconfig.h"
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

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id){
        char aux[500];
        case HTTP_EVENT_ON_DATA:
            sprintf(aux, "%.*s\n", evt->data_len, (char *)evt->data);
            ESP_LOGI("GET", ": valor enviado: %s", aux);
            break;

        default:
            break;
    }
    return ESP_OK;
}

static void sensors_post_rest_function()
{
    esp_http_client_config_t config_post = {
        .url = "http://192.168.4.1/api/v1/temp/update",
        .method = HTTP_METHOD_POST,
        .cert_pem = NULL,
        .event_handler = client_event_post_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    float temp = 20.145;
    float hum = 163.245;
    char buf[100];
    sprintf(buf, "%.3f %.3f", temp, hum);
    comPackage = createPackage(0x5A, 0xAE, strlen((const char*) buf), (uint8_t*) buf, 0xB2);
    char data_to_send[255];
    packageEncode(data_to_send, comPackage);
    
    cJSON *data = cJSON_CreateObject();
    cJSON *str = cJSON_CreateString(data_to_send);

    cJSON_AddItemToObject(data, "node", str);

    const char *info = cJSON_Print(data);
    ESP_LOGI("POST", ": valor a enviar: %s", info);
    esp_http_client_set_post_field(client,info, strlen(info));
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
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
            ESP_LOGI("GET", ": valor recibido: \n");
            decodeValue(comPackage.data);
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
        .event_handler = client_event_get_handler};
        
    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void app_main(void){
    nvs_flash_init();
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n\n");
    while(1){
        sensors_post_rest_function();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        sensors_rest_get();
    }
}