typedef struct Package {
    uint8_t header, command, length, end;
    uint8_t *data;
    uint32_t crc32;
} Protocol;

void array_append(char *arr, int size, char value);

uint32_t crc32b(char *message);

Protocol createPackage(uint8_t header, uint8_t command, uint8_t length, uint8_t *data, uint8_t end);
Protocol packageDecode(char *str);
void packageEncode(char *str, Protocol pkg);
void printPackage(Protocol pkg);

Protocol comPackage;