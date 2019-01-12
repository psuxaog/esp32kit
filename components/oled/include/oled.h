#ifndef __OLED_H__
#define __OLED_H__
#include "driver/i2c.h"
#include "sdkconfig.h"
/*
 * IO Map Configure
 */
#define OLED_XSIZE  128
#define OLED_YSIZE  64
typedef struct
{
	uint8_t xSize;
	uint8_t ySize;
	uint8_t *pTable;
} FONT_TypeDef;
typedef uint8_t color_t;
extern void OLED_Init(void);
extern void OLED_Enable(uint8_t state);
extern void OLED_SetLight(uint8_t light);
extern void OLED_UpdateScreen(void);
extern void OLED_DrawHLine(uint8_t x, uint8_t y, uint8_t size, color_t color);
extern void OLED_Clear(void);
extern void OLED_DrawAscString8x16(uint8_t x, uint8_t y, char *str,
		color_t color);
extern void OLED_DrawAscString7x14(uint8_t x, uint8_t y, char *str,
		color_t color);
extern void OLED_DrawAscString5x7(uint8_t x, uint8_t y, char *str,
		color_t color);
extern void OLED_DrawAscString5x7Len(uint8_t x, uint8_t y, char *str,
		color_t color, uint8_t len);
extern void OLED_DrawHz20x20(uint8_t x, uint8_t y, uint8_t id, color_t color);
extern void OLED_DrawHz14x14(uint8_t x, uint8_t y, uint8_t id, color_t color);
extern void OLED_DrawBMP(uint8_t x, uint8_t y, const uint8_t *bmp,
		color_t color);
extern void OLED_Print(const char *content, ...);
#endif
