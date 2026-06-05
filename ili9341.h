#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include "main.h"
#include <stdint.h>

#define ILI9341_WIDTH   240
#define ILI9341_HEIGHT  320

#define ILI9341_BLACK   0x0000
#define ILI9341_WHITE   0xFFFF
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_BLUE    0x001F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_ORANGE  0xFD20

void ILI9341_Init(void);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawUpArrow(uint16_t color);
void ILI9341_DrawDownArrow(uint16_t color);
void ILI9341_DrawLeftArrow(uint16_t color);
void ILI9341_DrawRightArrow(uint16_t color);

/* Text functions */
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif
