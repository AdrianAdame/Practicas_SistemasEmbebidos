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

#define _DEF_REG_32b( addr ) ( *(volatile uint32_t *)( addr ) )
#define GPIO_OUT_REG        _DEF_REG_32b( 0x3FF44004 )
#define GPIO_ENABLE_REG     _DEF_REG_32b( 0x3FF44020 )

#define EXAMPLE_ESP_WIFI_SSID      "temp"
#define EXAMPLE_ESP_WIFI_PASS      "123"
#define EXAMPLE_MAX_STA_CONN       2

static const char *TAG = "softAP_WebServer";


void command_process(char *command, char *string, char *cmd_selected){
    GPIO_OUT_REG |= (1 << 2);

    srand(time(NULL));
    ESP_LOGI(TAG, "Comando recibido: %s\n", command);
    int comand = strtol(command, NULL, 16);

    ESP_LOGI(TAG, "Comando seleccionado: 0x%02X\n", comand);
    
    switch (comand){
        case 0x10:
            itoa(xTaskGetTickCount(),string,10);
            ESP_LOGI("TAG", "Comando 0x10\n\tTimestamp: %s\n", string);
            strcpy(cmd_selected, "Timestamp");
            break;
        case 0x11:
            itoa((GPIO_ENABLE_REG >> 2) & 1, string, 10);
            strcpy(cmd_selected, "Estado de LED");
            ESP_LOGI("TAG", "Comando 0x11\n\tEstado de LED: %s\n", string);
            break;
        case 0x12:
            itoa(rand()%30, string, 10);
            strcpy(cmd_selected, "Temperatura");
            ESP_LOGI("TAG", "Comando 0x12\n\tTemperatura: %s\n", string);
            break;
        case 0x13:
            if((GPIO_ENABLE_REG >> 2) & 1) 
                GPIO_ENABLE_REG &= ~(1 << 2);
            else 
                GPIO_ENABLE_REG |= 1 << 2;
            strcpy(cmd_selected, "Invertir LED");
            strcpy(string, "Led Invertido");
            ESP_LOGI("TAG", "Comando 0x13\n\t: %s\n", string);
            break;
        default:
            break;
    }
}


/* An HTTP GET handler */
static esp_err_t index_get_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[5];
            char string[100];

            if (httpd_query_key_value(buf, "command", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => %s", param);
                command_process(param, string, NULL);

                httpd_resp_set_hdr(req, "Response", string);

                const char* resp_str = (const char*) req->user_ctx;
                httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
            }
        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t index_get = {
    .uri       = "/index",
    .method    = HTTP_GET,
    .handler   = index_get_handler,
    .user_ctx  = "<h1>27.89</h1>"
};

static esp_err_t index_post_handler(httpd_req_t *req){
    char content[100];
    char string[100];
    char cmd_selected[100];
    char ctx[500];

    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);

    if(ret <= 0){
        if(ret == HTTPD_SOCK_ERR_TIMEOUT){
            httpd_resp_send_408(req);
        }

        return ESP_FAIL;
    }

    char resp[5] = {content[8],content[9],content[10],content[11], '\0'}; 

    command_process(resp, string, cmd_selected);

    sprintf(ctx, "<h1>%s</h1>", resp);

    httpd_resp_send(req, ctx, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t index_post = {
    .uri = "/index",
    .method = HTTP_POST,
    .handler = index_post_handler,
    .user_ctx = "<form action='index' method='post'><label for='command'>Ingresar comando</label><input type='text' id='command' name='command'></input><input type='submit' value='Enviar comando'></form>"
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Iniciar el servidor httpd 
    ESP_LOGI(TAG, "Iniciando el servidor en el puerto: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Manejadores de URI
        ESP_LOGI(TAG, "Registrando manejadores de URI");
        httpd_register_uri_handler(server, &index_get);
        httpd_register_uri_handler(server, &index_post);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "estacion "MACSTR" se unio, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "estacion "MACSTR" se desconecto, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

esp_err_t wifi_init_softap(void){
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Inicializacion de softAP terminada. SSID: %s password: %s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    return ESP_OK;
}


void app_main(void)
{
    httpd_handle_t server = NULL;
    
    ESP_ERROR_CHECK(nvs_flash_init());
    
    esp_netif_init();
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_LOGI(TAG, "init softAP");
    
    ESP_ERROR_CHECK(wifi_init_softap());
    
    server = start_webserver();
}