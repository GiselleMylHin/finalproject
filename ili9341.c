/*
 * ili9341.c
 *
 *  Created on: May 26, 2026
 *      Author: Jessica Sierra
 */
/* LCD control pins */

#include "ili9341.h"

#define LCD_DC_PIN      GPIO_PIN_3
#define LCD_CS_PIN      GPIO_PIN_4
#define LCD_RST_PIN     GPIO_PIN_8

#define CS_LOW()        (GPIOC->BRR  = LCD_CS_PIN)
#define CS_HIGH()       (GPIOC->BSRR = LCD_CS_PIN)

#define DC_CMD()        (GPIOC->BRR  = LCD_DC_PIN)
#define DC_DATA()       (GPIOC->BSRR = LCD_DC_PIN)

#define RST_LOW()       (GPIOC->BRR  = LCD_RST_PIN)
#define RST_HIGH()      (GPIOC->BSRR = LCD_RST_PIN)

static void LCD_SetAddressWindow(uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1);
static void LCD_SPI_WriteByte(uint8_t data);
static void LCD_SPI_WaitDone(void);

static void LCD_SPI_WriteByte(uint8_t data)
{
    while (!(SPI1->SR & SPI_SR_TXE))
        ;

    *((volatile uint8_t *)&SPI1->DR) = data;
}

static void LCD_SPI_WaitDone(void)
{
    while (!(SPI1->SR & SPI_SR_TXE))
        ;

    while (SPI1->SR & SPI_SR_BSY)
        ;
}

static void LCD_WriteCommand(uint8_t cmd)
{
    DC_CMD();
    CS_LOW();

    LCD_SPI_WriteByte(cmd);
    LCD_SPI_WaitDone();

    CS_HIGH();
}

static void LCD_WriteData(uint8_t data)
{
    DC_DATA();
    CS_LOW();

    LCD_SPI_WriteByte(data);
    LCD_SPI_WaitDone();

    CS_HIGH();
}

static void LCD_Reset(void)
{
    RST_LOW();
    HAL_Delay(20);

    RST_HIGH();
    HAL_Delay(120);
}

static void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
    LCD_WriteCommand(0x2A);
    LCD_WriteData(x0 >> 8);
    LCD_WriteData(x0 & 0xFF);
    LCD_WriteData(x1 >> 8);
    LCD_WriteData(x1 & 0xFF);

    LCD_WriteCommand(0x2B);
    LCD_WriteData(y0 >> 8);
    LCD_WriteData(y0 & 0xFF);
    LCD_WriteData(y1 >> 8);
    LCD_WriteData(y1 & 0xFF);

    LCD_WriteCommand(0x2C);
}

void ILI9341_Init(void){
    CS_HIGH();
    RST_HIGH();

    LCD_Reset();

    LCD_WriteCommand(0x01);
    HAL_Delay(150);

    LCD_WriteCommand(0x28);

    LCD_WriteCommand(0x3A);
    LCD_WriteData(0x55);

    LCD_WriteCommand(0x36);
    LCD_WriteData(0x48);

    LCD_WriteCommand(0x11);
    HAL_Delay(120);

    LCD_WriteCommand(0x29);
    HAL_Delay(20);
}

void ILI9341_FillScreen(uint16_t color){
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    LCD_SetAddressWindow(0, 0, ILI9341_WIDTH - 1, ILI9341_HEIGHT - 1);

    DC_DATA();
    CS_LOW();

    for (uint32_t i = 0; i < ILI9341_WIDTH * ILI9341_HEIGHT; i++){
        LCD_SPI_WriteByte(hi);
        LCD_SPI_WriteByte(lo);
    }

    LCD_SPI_WaitDone();
    CS_HIGH();
}

void ILI9341_FillRect(uint16_t x, uint16_t y,uint16_t w, uint16_t h,uint16_t color){
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    LCD_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    DC_DATA();
    CS_LOW();

    for (uint32_t i = 0; i < (uint32_t)w * h; i++){
        LCD_SPI_WriteByte(hi);
        LCD_SPI_WriteByte(lo);
    }

    LCD_SPI_WaitDone();
    CS_HIGH();
}

void ILI9341_DrawUpArrow(uint16_t color)
{
    ILI9341_FillScreen(ILI9341_BLACK);

    /* Arrow head: point at top */
    for (int i = 0; i < 50; i++)
    {
        ILI9341_FillRect(120 - i, 50 + i, (2 * i) + 1, 1, color);
    }

    /* Shaft */
    ILI9341_FillRect(115, 100, 10, 120, color);
}

void ILI9341_DrawDownArrow(uint16_t color)
{
    ILI9341_FillScreen(ILI9341_BLACK);

    /* Arrow head: point at bottom */
    for (int i = 0; i < 50; i++)
    {
        ILI9341_FillRect(120 - i, 220 - i, (2 * i) + 1, 1, color);
    }

    /* Shaft */
    ILI9341_FillRect(115, 50, 10, 120, color);
}

void ILI9341_DrawLeftArrow(uint16_t color)
{
    ILI9341_FillScreen(ILI9341_BLACK);

    /* Arrow head: point at left */
    for (int i = 0; i < 50; i++)
    {
        ILI9341_FillRect(50 + i, 120 - i, 1, (2 * i) + 1, color);
    }

    /* Shaft */
    ILI9341_FillRect(100, 115, 100, 10, color);
}

void ILI9341_DrawRightArrow(uint16_t color)
{
    ILI9341_FillScreen(ILI9341_BLACK);

    /* Arrow head: point at right */
    for (int i = 0; i < 50; i++)
    {
        ILI9341_FillRect(190 - i, 120 - i, 1, (2 * i) + 1, color);
    }

    /* Shaft */
    ILI9341_FillRect(40, 115, 100, 10, color);
}
