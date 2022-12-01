#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"
#include "myUart.h"
#include "communication_protocol.h"

#define SPP_DATA_LEN            20 

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXAMPLE_DEVICE_NAME "ESP_SPP_ACCEPTOR2"
 
static bool bWriteAfterOpenEvt = true;
static bool bWriteAfterWriteEvt = false;
static bool bWriteAfterSvrOpenEvt = true;
static bool bWriteAfterDataReceived = true;
 
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB; 
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
// static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_MASTER;

static uint8_t spp_data[SPP_DATA_LEN];
char cad[20];

uint8_t *createTestBuffer(void){
    static const uint8_t buffer[] = { 5, 7, 3, 4, 9, 1, 3 };
    uint8_t *rv = malloc(sizeof(buffer));
    if (rv != 0)
        memmove(rv, buffer, sizeof(buffer));
    return rv;
}
 
static char charArrayLastReceivedData[18];
 
static void saveReceivedData(int len, uint8_t *p_data){
    char array1[18] = "abcdefg";
    char array2[18];
    strncpy(array2, array1, 18);
    strncpy(charArrayLastReceivedData, array1, 18);
}
 
static int getDataToSend(int *len, uint8_t *p_data){
    return 0;   //ok
}
 
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
    //Used in app_main() to setup the BT configuration in the ESP32 and used for communication with device
    switch (event) {
    case ESP_SPP_INIT_EVT:
        esp_bt_dev_set_device_name("EDGAR");
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        break;
    case ESP_SPP_OPEN_EVT:
        if (bWriteAfterOpenEvt){
            //esp_err_tesp_spp_write(uint32_t handle, int len, uint8_t *p_data)
            esp_spp_write(param->srv_open.handle, SPP_DATA_LEN, spp_data);
 
            char * c = "Hello";
            sprintf(c, "test");
            uint8_t * u = (uint8_t *)c;
 
            esp_spp_write(param->srv_open.handle, 6, u);
        }
        else{
            // ESP_LOGI(SPP_TAG, "bWriteAfterOpenEvt = false");
        }
        break;
    case ESP_SPP_CLOSE_EVT:
        break;
    case ESP_SPP_START_EVT:                                         //Short before connection is established
        break;
    case ESP_SPP_CL_INIT_EVT:
        break;
    case ESP_SPP_DATA_IND_EVT:                                      //When SPP connection received data, the event comes, only for ESP_SPP_MODE_CB
        // esp_log_buffer_hex("Received HEX Data",param->data_ind.data,param->data_ind.len);
        esp_log_buffer_char("Device ",param->data_ind.data,param->data_ind.len);
        saveReceivedData(param->data_ind.len, param->data_ind.data);
        comPackage = packageDecode((char *) param->data_ind.data);
        printPackage(comPackage);
        if (bWriteAfterDataReceived){
            char c[20] = "";             

            // ESP_LOGE(SPP_TAG,"ESP: ");
            uartPuts(0, "MY ESP: ");
            uartGets(0, cad);
            // sprintf(c,"%s", cad);
            // strcpy(cad, c);

            // uint8_t * u = (uint8_t *)c;
            // esp_spp_write(param->srv_open.handle, strlen(cad), u);

            char data_to_send[100];
            comPackage = createPackage(0x5A, 0x5B, strlen(cad), (uint8_t*)cad, 0xB2);
            packageEncode(data_to_send, comPackage);
            esp_spp_write(param->srv_open.handle, strlen(data_to_send), (uint8_t*)data_to_send);
        }
        else{
            // ESP_LOGI(SPP_TAG, "bWriteAfterDataReceived = false");
        }
 
        break;
    case ESP_SPP_CONG_EVT:
        // ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        //ToDo: Now the next line is incorrect, it shows not the data which was sent!
        // esp_log_buffer_hex("HEX Data was sent",spp_data,SPP_DATA_LEN);
         if (param->write.cong == 0) {
            if (bWriteAfterWriteEvt){
                esp_spp_write(param->write.handle, SPP_DATA_LEN, spp_data);
            }
            else{
                // ESP_LOGI(SPP_TAG, "bWriteAfterWriteEvt = false");
            }
        }
        else {
            // ESP_LOGI(SPP_TAG, "param->write.cong <> 0");
        } 
        break;
    case ESP_SPP_SRV_OPEN_EVT:                                      //After connection is established, short before data is received
        if (bWriteAfterSvrOpenEvt){
            char c[20];             // "Hello" is in fact just { 'H', 'e', 'l', 'l', 'o', '\0' }
            // ESP_LOGE(SPP_TAG,"ESP: ");
            uartPuts(0, "MY ESP:");
            uartGets(0, cad);
            // sprintf(c,"%s", cad);
            // strcpy(cad, c);
            // uint8_t * u = (uint8_t *)c;
            // esp_spp_write(param->srv_open.handle, strlen(cad), u);    //Works, but maybe it needs something like CR
       
            char data_to_send[100];
            comPackage = createPackage(0x5A, 0x5B, strlen(cad), (uint8_t*)cad, 0xB2);
            packageEncode(data_to_send, comPackage);
            esp_spp_write(param->srv_open.handle, strlen(data_to_send), (uint8_t*)data_to_send);
        }
        else{
            // ESP_LOGI(SPP_TAG, "bWriteAfterSvrOpenEvt = false");
        }
 
        break;
    default:
        break;
    }
}
 
void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param){
    //Used in app_main() to setup the BT configuration in the ESP32
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        if (param->pin_req.min_16_digit) {
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            // ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

    case ESP_BT_GAP_CFM_REQ_EVT:
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        break;
    default: {
        break;
    }
    }
    return;
}
 
void startClassicBtSpp(void){

    for (int i = 0; i < strlen(cad); ++i) {
        spp_data[i] = cad[i];
    }

    esp_err_t ret = nvs_flash_init();   //Initialize the default NVS partition.
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));        //release the controller memory as per the mode 
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    } else {
        // ESP_LOGI(SPP_TAG, "Initialize controller ok");
    }
   
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    } else {
        // ESP_LOGI(SPP_TAG, "Enable controller ok");
    }
   
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    } else {
        // ESP_LOGI(SPP_TAG, "Initialize bluedroid ok");
    }
   
    // ESP_LOGI(SPP_TAG, "Call esp_bluedroid_enable()");
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    else{
        // ESP_LOGI(SPP_TAG, "Enable bluedroid ok");
    }
   
    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    } else {
        // ESP_LOGI(SPP_TAG, "Gap register ok");
    }
   
    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    else{
        ESP_LOGI(SPP_TAG, "spp register ok");
    }
   
    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    else{
        // ESP_LOGI(SPP_TAG, "spp init ok");
    }
   
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code); 
}
 
void app_main(void){
    uartInit(0, 115200, 8, 0, 0);
    startClassicBtSpp();
}