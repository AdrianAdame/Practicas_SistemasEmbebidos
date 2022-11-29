#include "esp_log.h"
#include "stdlib.h"
#include "string.h"
#include "communication_protocol.h"

void array_append(char *arr, int size, char value){
    arr[size] = value;
}

uint32_t crc32b(char *message){
    int i, j;
    unsigned int byte, crc, mask;

    i = 0;
    crc = 0xFFFFFFFF;
    while(message[i] != 0xB2){
        byte = message[i];
        crc = crc ^ byte;

        for(j = 7; j >= 0; j --){
            mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }

        i++;
    }

    return ~crc;
}

Protocol createPackage(uint8_t header, uint8_t command, uint8_t length, uint8_t *data, uint8_t end){
    Protocol pkg;

    pkg = (Protocol) {
        .header = header,
        .command = command,
        .length = length,
        .end = end,
        .crc32 = 0
    };

    if(length > 0){
        pkg.data = (uint8_t*)malloc(sizeof(uint8_t) * length);
        memcpy(pkg.data, data, length + 1);
    }else
        pkg.data = NULL;

    return pkg;
}

Protocol packageDecode(char *str){
    Protocol pkg;
    int i = 0;
    uint8_t position = 0, auxVar = 0;
    uint32_t crc32 = 0;

    pkg.header = ((uint8_t) str[position++]);
    pkg.command = ((uint8_t) str[position++]);
    pkg.length = ((uint8_t) str[position++]);

    if(pkg.length > 0){
        pkg.data = (uint8_t*) malloc (sizeof(uint8_t) * pkg.length + 1);
        for(; i < pkg.length; i++)
            pkg.data[i] = str[position++];
        
        pkg.data[i] = '\0';
    }else{
        pkg.data = NULL;
    }

    pkg.end = ((uint8_t) str[position]);

    for(int  i = 0; i < 3; i++){
        auxVar = ((uint8_t) str[position++]);
        crc32 |= auxVar;
        crc32 <<= 8;
    }

    pkg.crc32 = ((crc32 >> 24) & 0xFF) | ((crc32 << 8) & 0xFF0000) | ((crc32 >> 8) & 0xFF00) | ((crc32 << 24) & 0xFF000000);

    return pkg;
}

void packageEncode(char *str, Protocol pkg){
    int size = 0;
    uint32_t crc32 = 0;

    array_append(str, size++, (char) pkg.header);
    array_append(str, size++, (char) pkg.command);

    array_append(str, size++, (char)(pkg.length == 0x0 ? -0x1 : pkg.length));

    for(int i = 0; i < pkg.length; i++)
        array_append(str, size++, (char)pkg.data[i]);
    
    array_append(str, size++, (char) pkg.end);

    pkg.crc32 = crc32b(str);

    crc32 = pkg.crc32;

    for(int i = 0; i < 4; i++){
        array_append(str, size++, (char) crc32);
        crc32 >>= 8;
    }
}

void printPackage(Protocol pkg){
    ESP_LOGI("Header", ": 0x%01X\n", pkg.header);
    ESP_LOGI("Command", ": 0x%01X\n", pkg.command);
    ESP_LOGI("Length", ": 0x%01X\n", pkg.length);
    if(pkg.length > 0)
        ESP_LOGI("*Data", ": %s\n", pkg.data);
    else
        ESP_LOGI("*Data", ": %s\n","NULL");
    
    ESP_LOGI("End", ": 0x%01X\n", pkg.end);
    ESP_LOGI("CRC32", ": 0x%01X\n", pkg.crc32);
}

void decodeValue(uint8_t *pkgValue){
    char* pend;
    float f1 = strtof((char*)pkgValue, &pend);
    float f2 = strtof(pend, NULL);
    ESP_LOGI("Valores", "Temp: %.3f | Humedad: %.3f\n", f1, f2);
}