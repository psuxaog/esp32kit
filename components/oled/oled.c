#include "oled.h"
#include "fonts.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
static uint8_t OLED_GRAM[OLED_YSIZE / 8][OLED_XSIZE];

#define OLED_DELAY(x) HAL_Delay(x)

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define DATA_LENGTH 512                  /*!< Data buffer length of test buffer */
#define RW_TEST_LENGTH 128               /*!< Data length for r/w test, [0,DATA_LENGTH] */
#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

#define I2C_MASTER_SCL_IO 15               /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 4               /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUMBER(1) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 400000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define ESP_SLAVE_ADDR 0x78 /*!< ESP32 slave address, you can set any 7bit value */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

static esp_err_t OLED_WriteCmd(uint8_t pCmd)
{
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ESP_SLAVE_ADDR, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, pCmd, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}
static esp_err_t OLED_WriteData(uint8_t *pData, uint8_t len)
{
	int ret;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ESP_SLAVE_ADDR, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x40, ACK_CHECK_EN);
	i2c_master_write(cmd, pData, len, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}
void OLED_Clear(void)
{
	uint8_t i, n;
	for (i = 0; i < (OLED_YSIZE / 8); i++)
	{
		for (n = 0; n < OLED_XSIZE; n++)
		{
			OLED_GRAM[i][n] = 0;
		}
	}
}
void OLED_DrawPixel(uint8_t x, uint8_t y, color_t color)
{
	uint8_t pos, bx, temp = 0;
	if ((x >= OLED_XSIZE) || (y >= OLED_YSIZE))
	{
		return;
	}
	pos = y / 8;
	bx = y % 8;
	temp = 1 << bx;
	if (color)
	{
		OLED_GRAM[pos][x] |= temp;
	}
	else
	{
		OLED_GRAM[pos][x] &= ~temp;
	}
}

void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t size, color_t color)
{
	uint8_t i;
	for (i = 0; i < size; i++)
	{
		OLED_DrawPixel(x + i, y, color);
	}
}
void OLED_DrawClearLine(uint8_t x, uint8_t y, color_t color)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_DrawHLine(0, y + i, 128, color);
	}
}
void OLED_DrawAscString8x16(uint8_t x, uint8_t y, char *str, color_t color)
{
	uint8_t i, z, j = 0;
	while (*str)
	{
		for (z = 0; z < 16; z++)
		{
			for (i = 0; i < 8; i++)
			{
				if (FONT_gAscTable8x16[*str - ' '][z] & (0x80 >> i))
				{
					OLED_DrawPixel(i + x + j * 8, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 8, z + y, !color);
				}
			}
		}
		j++;
		str++;
	}
}
void OLED_DrawAscString5x7(uint8_t x, uint8_t y, char *str, color_t color)
{
	uint8_t i, z, j = 0;
	while (*str)
	{
		for (z = 0; z < 7; z++)
		{
			for (i = 0; i < 5; i++)
			{
				if (FONT_gAscTable5x7[*str - ' '][z] & (0x10 >> i))
				{
					OLED_DrawPixel(i + x + j * 6, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 6, z + y, !color);
				}
			}
		}
		j++;
		str++;
	}
}
void OLED_DrawAscString5x7Len(uint8_t x, uint8_t y, char *str, color_t color,
    uint8_t len)
{
	uint8_t i, z, j = 0;
	while (*str)
	{
		if (len == 0)
		{
			return;
		}
		len--;
		if(*str <' '||*str > '~')
		{
			str ++;
			continue;
		}
		for (z = 0; z < 7; z++)
		{
			for (i = 0; i < 5; i++)
			{
				if (FONT_gAscTable5x7[*str - ' '][z] & (0x10 >> i))
				{
					OLED_DrawPixel(i + x + j * 6, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 6, z + y, !color);
				}
			}
		}
		j++;
		str++;
	}
}
void OLED_DrawAscString7x14(uint8_t x, uint8_t y, char *str, color_t color)
{
	uint8_t i, z, j = 0;
	while (*str)
	{
		for (z = 0; z < 14; z++)
		{
			for (i = 0; i < 7; i++)
			{
				if (FONT_gAscTable7x14[*str - ' '][z] & (0x80 >> i))
				{
					OLED_DrawPixel(i + x + j * 7, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 7, z + y, !color);
				}
			}
		}
		j++;
		str++;
	}
}
void OLED_DrawHz20x20(uint8_t x, uint8_t y, uint8_t id, color_t color)
{
	uint8_t i, z, j = 0;
	for (z = 0; z < 20; z++)
	{
		for (j = 0; j < 3; j++)
		{
			for (i = 0; i < 8; i++)
			{
				if (FONT_gHzTable20x20[id][z * 3 + j] & (0x80 >> i))
				{
					OLED_DrawPixel(i + x + j * 8, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 8, z + y, !color);
				}
			}
		}
	}
}
void OLED_DrawHz14x14(uint8_t x, uint8_t y, uint8_t id, color_t color)
{
	uint8_t i, z, j = 0;
	for (z = 0; z < 14; z++)
	{
		for (j = 0; j < 2; j++)
		{
			for (i = 0; i < 7; i++)
			{
				if (FONT_gHzTable14x14[id][z * 2 + j] & (0x80 >> i))
				{
					OLED_DrawPixel(i + x + j * 7, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i + x + j * 7, z + y, !color);
				}
			}
		}
	}
}
void OLED_DrawBMP(uint8_t x, uint8_t y, const uint8_t *bmp, color_t color)
{
	uint8_t i, j, z;
	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 8; j++)
		{
			for (z = 0; z < OLED_YSIZE; z++)
			{
				if (bmp[i + z * 16] & (0x80 >> j))
				{
					OLED_DrawPixel(i * 8 + j + x, z + y, color);
				}
				else
				{
					OLED_DrawPixel(i * 8 + j + x, z + y, !color);
				}
			}
		}
	}
}
void OLED_UpdateScreen(void)
{
	uint8_t i;
	for (i = 0; i < (OLED_YSIZE / 8); i++)
	{
		OLED_WriteCmd(0xB0 + i);
		OLED_WriteCmd(0x01);
		OLED_WriteCmd(0x10);
		OLED_WriteData(OLED_GRAM[i], OLED_XSIZE);
	}
}
void OLED_Enable(uint8_t state)
{
	if (state)
	{
		OLED_WriteCmd(0xAF);
	}
	else
	{
		OLED_WriteCmd(0xAE);
	}
}
void OLED_Print(const char *content, ...)
{
	static int currentLine = 0;
	int maxLine = 0;
	uint8_t i;
	char *pBuffer = pvPortMalloc(100);
	va_list ap;
	va_start(ap, content);
	vsprintf(pBuffer, content, ap);
	va_end(ap);
	maxLine = (strlen(pBuffer) / 21) + 1;
	for (i = 0; i < maxLine; i++)
	{
		OLED_DrawAscString5x7Len(0, currentLine * 8, &pBuffer[i * 21], 1, 21);
		OLED_DrawClearLine(0, (currentLine + 1) * 8, 0);
		currentLine++;
		if (currentLine > 7)
		{
			currentLine = 0;
		}
	}
	vPortFree(pBuffer);
	OLED_UpdateScreen();
}
void OLED_SetLight(uint8_t light)
{
	OLED_WriteCmd(0x81);
	OLED_WriteCmd(light);
}
/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init()
{
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	i2c_param_config(i2c_master_port, &conf);
	return i2c_driver_install(i2c_master_port, conf.mode,
	I2C_MASTER_RX_BUF_DISABLE,
	I2C_MASTER_TX_BUF_DISABLE, 0);
}

void OLED_Init(void)
{
	ESP_ERROR_CHECK(i2c_master_init());
	gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_16, 0);
	vTaskDelay(100 / portTICK_PERIOD_MS);
	gpio_set_level(GPIO_NUM_16, 1);
	OLED_WriteCmd(0xae); //--turn off oled panel
	OLED_WriteCmd(0x00); //---set low column address
	OLED_WriteCmd(0x10); //---set high column address
	OLED_WriteCmd(0x40); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WriteCmd(0x81); //--set contrast control register
	OLED_WriteCmd(0xCF); // Set SEG Output Current Brightness
	OLED_WriteCmd(0xa1); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WriteCmd(0xc8); //Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WriteCmd(0xa6); //--set normal display
	OLED_WriteCmd(0xa8); //--set multiplex ratio(1 to 64)
	OLED_WriteCmd(0x3f); //--1/64 duty
	OLED_WriteCmd(0xd3); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WriteCmd(0x00); //-not offset
	OLED_WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
	OLED_WriteCmd(0x80); //--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WriteCmd(0xd9); //--set pre-charge period
	OLED_WriteCmd(0xf1); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WriteCmd(0xda); //--set com pins hardware configuration
	OLED_WriteCmd(0x12);
	OLED_WriteCmd(0xdb); //--set vcomh
	OLED_WriteCmd(0x40); //Set VCOM Deselect Level
	OLED_WriteCmd(0x20); //-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WriteCmd(0x02); //
	OLED_WriteCmd(0x8d); //--set Charge Pump enable/disable
	OLED_WriteCmd(0x14); //--set(0x10) disable
	OLED_WriteCmd(0xa4); // Disable Entire Display On (0xa4/0xa5)
	OLED_WriteCmd(0xa6); // Disable Inverse Display On (0xa6/a7)
	OLED_Clear();

	OLED_UpdateScreen();
	OLED_WriteCmd(0xAF); //Set Display On
}
