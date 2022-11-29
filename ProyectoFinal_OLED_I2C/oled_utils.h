#include "oled_resources.h"
#include "string.h"
#define tag "SSD1306"
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_NUM 0
#define OLED_I2C_ADDRESS   0x3C




//CAMBIAR FUENTE, CANTIDAD DE COLUMNAS Y TAMANO DE LOS CARACTERES
#define ASCII8X8 0
#define ASCII6X8 1
#define FONT ASCII6X8
#if FONT == ASCII6X8
	#define COLUMNAS 21 
	#define CHAR_SIZE 6
	#define ASCII_FONT ssd1306xled_font6x8
#else
	#define COLUMNAS 16
	#define CHAR_SIZE 8
	#define ASCII_FONT ssd1306xled_font8x8
#endif

/*
Pantalla oled SSD1306 datasheet pagina 25
+----------------------------------------+
|                 Page 0                 | AMARILLO
+----------------------------------------+
|                 Page 1                 | AMARILLO
+----------------------------------------+
|                 Page 2                 | AZUL
+----------------------------------------+
|                 Page 3                 | AZUL
+----------------------------------------+
|                 Page 4                 | AZUL
+----------------------------------------+
|                 Page 5                 | AZUL
+----------------------------------------+
|                 Page 6                 | AZUL
+----------------------------------------+
|                 Page 7                 | AZUL
+----------------------------------------+

La pantalla se divide en 8 paginas de arriba hacia abajo
Se tienen 16 o 21 columnas debido a la codificacion de los caracteres
Por lo que tenemos una pantalla util de 16*8 o 21*8 caracteres

Debido a que se tiene el comando de cargar datos desde la RAM, si
se desconectan los cables SDA y SCL el contenido se queda en pantalla
mientras exista VCC y GND

Para desplegar imagenes checar AS
*/




/**
 * @brief Configuracion i2c con la pantalla oled
 * 
 * @return esp_err_t 
 */
static esp_err_t i2c_oled_master_init(){
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = I2C_MASTER_SDA_IO,
		.scl_io_num = I2C_MASTER_SCL_IO,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 1000000
	};
	i2c_param_config(i2c_master_port, &conf);
	return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}
/**
 * @brief Manda comandos a la pantalla, checar pagina 28 del SSD1306 datasheet para ver comandos disponibles
 * 
 * @param command 
 * @return esp_err_t 
 */
esp_err_t oled_send_command(uint8_t command){
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 0);
	i2c_master_write_byte(cmd,0x00, 0);
	i2c_master_write_byte(cmd,command, 0);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

/**
 * @brief Manda datos a la pantalla, no confundir con comandos
 * 
 * @param data 
 * @param size 
 * @return esp_err_t 
 */
esp_err_t oled_send_data(uint8_t *data,uint16_t size){
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 0);
	i2c_master_write_byte(cmd,0x40, 0);
	i2c_master_write(cmd,data,size, 0);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

/**
 * @brief Hace las configuraciones necesarias para poder utilizar la pantalla. Usa oled_send_command para la configuracion
 * 
 */
void oled_init() {

	oled_send_command(0xAE);	//Display OFF (sleep mode)(RESET) 

	oled_send_command(0x81);	//Double byte command to select 1 
	oled_send_command(0xFF);	//out of 256 contrast steps

	oled_send_command(0x20);	//Set Memory Addressing Mode 
	oled_send_command(0x00);	//A[1:0] = 00b, Horizontal Addressing Mode

	oled_send_command(0xB0);	//Set GDDRAM Page Start Address (PAGE0~PAGE7) for 
								//Page Addressing Mode using X[2:0]. 

	oled_send_command(0xA4);	//A4h, X0=0b: Resume to RAM content display

	oled_send_command(0xA6);	//A6h, X[0]=0b: Normal display (RESET)

	oled_send_command(0xC8);	//COM Output Scan Direction as normal


	oled_send_command(0x40);	//Set display RAM display start line register 
								//from 0-63 using X5X3X2X1X0

	oled_send_command(0xA1);	//A1h, X[0]=1b: column address 127 is mapped to SEG0

	oled_send_command(0xDB);	//VCOMH Deselect Level
	oled_send_command(0x20);	//0.77 x VCC (RESET)

	oled_send_command(0xDA);	//COM Pins Hardware Configuration 
	oled_send_command(0x12);	//Alternative

	oled_send_command(0x8D);	//Charge Pump Setting
	oled_send_command(0x14);	//14h ; Enable Charge Pump 

	oled_send_command(0x10);	//Higher Column Start Address for Page Addressing Mode

	oled_send_command(0xAF);	//Display ON in normal mode

}
/**
 * @brief Limpia toda la pantalla
 * 
 */
void oled_clear(void) {
	uint8_t zeros[128];

	for(int i = 0; i < 128; i++)	zeros[i] = 0x00;

	for(int page = 0; page < 8; page++){
		//Set Page Start Address for Page Addressing Mode (B0h~B7h) 
		oled_send_command(0xB0 + page);
		/*
		Set the lower nibble of the column start address
		register for Page Addressing Mode using X[3:0]
		as data bits.
		*/
		oled_send_command(0x00);
		/*
		Set the higher nibble of the column start address
		register for Page Addressing Mode using X[3:0]
		as data bits
		*/
		oled_send_command(0x10);
		oled_send_data(&zeros[0],128);
	}

}
/**
 * @brief Rellena toda la pantalla
 * 
 */
void oled_fill(void) 
{
	uint8_t ones[128];

	for(int i = 0; i < 128; i++)	ones[i] = 0xFF;

	
	for(int page = 0; page < 8; page++){
		//Set Page Start Address for Page Addressing Mode (B0h~B7h) 
		oled_send_command(0xB0 + page);
		/*
		Set the lower nibble of the column start address
		register for Page Addressing Mode using X[3:0]
		as data bits.
		*/
		oled_send_command(0x00);
		/*
		Set the higher nibble of the column start address
		register for Page Addressing Mode using X[3:0]
		as data bits
		*/
		oled_send_command(0x10);
		oled_send_data(&ones[0],128);
	}
}
/**
 * @brief Limpia la parte amarilla de la pantalla (Paginas 0-1)
 * 
 */
void oled_clear_yellow(void) 
{
	uint8_t zeros[128];

	for(int i = 0; i < 128; i++)	zeros[i] = 0x00;

	//Clear first 2 pages
	for(int page = 0; page < 2; page++){
		oled_send_command(0xB0 + page);
		oled_send_command(0x00);
		oled_send_command(0x10);
		oled_send_data(&zeros[0],128);
	}
}
/**
 * @brief Limpia la parte azul de la pantalla (Paginas 2-7)
 * 
 */
void oled_clear_blue(void) 
{
	uint8_t zeros[128];

	for(int i = 0; i < 128; i++)	zeros[i] = 0x00;

	//Clear last 6 pages
	for(int page = 2; page < 8; page++){
		oled_send_command(0xB0 + page);
		oled_send_command(0x00);
		oled_send_command(0x10);
		oled_send_data(&zeros[0],128);
	}
}

/**
 * @brief Rellena la parte amarilla de la pantalla (Paginas 0-1)
 * 
 */
void oled_fill_yellow(void) 
{
	uint8_t ones[128];

	for(int i = 0; i < 128; i++)	ones[i] = 0xFF;

	//Fill first 2 pages
	for(int page = 0; page < 2; page++){
		oled_send_command(0xB0 + page);
		oled_send_command(0x00);
		oled_send_command(0x10);
		oled_send_data(&ones[0],128);
	}
}
/**
 * @brief Rellena la parte azul de la pantalla (Paginas 2-7)
 * 
 */
void oled_fill_blue(void) 
{
	uint8_t ones[128];

	for(int i = 0; i < 128; i++)	ones[i] = 0xFF;

	//Fill last 6 pages
	for(int page = 2; page < 8; page++){
		oled_send_command(0xB0 + page);
		oled_send_command(0x00);
		oled_send_command(0x10);
		oled_send_data(&ones[0],128);
	}
}
/**
 * @brief Manda caracter a la pantalla que este codificado en ASCII_chars
 * 
 * @param c 
 */
void oled_write_char(char c){
	oled_send_data(ASCII_FONT[(uint8_t) c],CHAR_SIZE);
}
/**
 * @brief 
 * Escribe una cadena pasada por str a la pantalla a partir de la pagina indicada.
 * 
 * @param str 
 * @param start_page 
 */
void oled_write_str(char * str, int start_page){
	uint8_t column = 0, page = start_page;
	oled_send_command(0xB0 + page++);
	oled_send_command(0x00);
	oled_send_command(0x10);

	for(uint8_t i = 0; i < strlen(str); i++){
		//Salto de linea
		if(column == COLUMNAS || (str[i] == '\n' && page != 0)){
			oled_send_command(0xB0 + page++);
			oled_send_command(0x00);
			oled_send_command(0x10);
			column = 0;
		}
		//Texto
		if(column < COLUMNAS && str[i] != '\n'){
			oled_write_char(str[i]);
			column++;
		}
	}
}