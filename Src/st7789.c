#include "st7789.h"
#include "stm32f1xx_ll_spi.h"
#include "stm32f1xx_ll_gpio.h"
#include "stm32f1xx_ll_utils.h"
#include "spi.h"
#include "Fonts.h"

#define LCD_RESET_LOW()   LL_GPIO_ResetOutputPin(ST7789_RST_PORT, ST7789_RST_PIN)
#define LCD_RESET_HIGH()  LL_GPIO_SetOutputPin(ST7789_RST_PORT, ST7789_RST_PIN)
 
#define LCD_DC_LOW()     LL_GPIO_ResetOutputPin(ST7789_DC_PORT, ST7789_DC_PIN)
#define LCD_DC_HIGH()    LL_GPIO_SetOutputPin(ST7789_DC_PORT, ST7789_DC_PIN)
 
#define LCD_CS_LOW()     LL_GPIO_ResetOutputPin(ST7789_CS_PORT, ST7789_CS_PIN)
#define LCD_CS_HIGH()    LL_GPIO_SetOutputPin(ST7789_CS_PORT, ST7789_CS_PIN)


static void ST7789_WriteCommand(uint8_t cmd)
{
	LCD_CS_LOW();//selet
	LCD_DC_LOW();//cmd mode
	spi_send_byte(SPI1,cmd);
	LCD_CS_HIGH();//unselect
}

static void ST7789_WriteSmallData(uint8_t data)
{
	LCD_CS_LOW();//selet
	LCD_DC_HIGH();//data mode
	spi_send_byte(SPI1,data);
	LCD_CS_HIGH();//unselect
}

static void ST7789_WriteData(SPI_TypeDef *SPIx, uint8_t *buff, uint16_t buff_size)
{
    LCD_CS_LOW();//selet
	LCD_DC_HIGH();//data mode

    for(uint16_t i = 0; i < buff_size; i++)
    {
        while(!LL_SPI_IsActiveFlag_TXE(SPIx));   // DR空
        LL_SPI_TransmitData8(SPIx, buff[i]);     // 写DR
    }

    while(LL_SPI_IsActiveFlag_BSY(SPIx));        // 等待发送完成

    LCD_CS_HIGH();//unselect
}

void ST7789_SetRotation(uint8_t m)
{
	ST7789_WriteCommand(ST7789_MADCTL);	// MADCTL
	switch (m) {
	case 0:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
		break;
	case 1:
		ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	case 2:
		ST7789_WriteSmallData(ST7789_MADCTL_RGB);
		break;
	case 3:
		ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	default:
		break;
	}
}

static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	LCD_CS_LOW();
	uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
	uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;
	
	/* Column Address set */
	ST7789_WriteCommand(ST7789_CASET); 
	{
		uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
		ST7789_WriteData(SPI1, data, sizeof(data));
	}

	/* Row Address set */
	ST7789_WriteCommand(ST7789_RASET);
	{
		uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
		ST7789_WriteData(SPI1, data, sizeof(data));
	}
	/* Write to RAM */
	ST7789_WriteCommand(ST7789_RAMWR);
	LCD_CS_HIGH();
}


void ST7789_Init(){
	//reset
	LL_mDelay(10);
	LCD_RESET_LOW();
	LL_mDelay(10);
	LCD_RESET_HIGH();
	LL_mDelay(20);
	
	
  	ST7789_WriteCommand(0xB2);				//	Porch control
	{
		uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		ST7789_WriteData(SPI1, data, sizeof(data));
	}
	ST7789_SetRotation(ST7789_ROTATION);	//	MADCTL (Display Rotation)
	
	/* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);				//	Gate Control
    ST7789_WriteSmallData(0x35);			//	Default value
    ST7789_WriteCommand(0xBB);				//	VCOM setting
    ST7789_WriteSmallData(0x19);			//	0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);				//	LCMCTRL	
    ST7789_WriteSmallData (0x2C);			//	Default value
    ST7789_WriteCommand (0xC2);				//	VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);			//	Default value
    ST7789_WriteCommand (0xC3);				//	VRH set
    ST7789_WriteSmallData (0x12);			//	+-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);				//	VDV set
    ST7789_WriteSmallData (0x20);			//	Default value
    ST7789_WriteCommand (0xC6);				//	Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);			//	Default value (60HZ)
    ST7789_WriteCommand (0xD0);				//	Power control
    ST7789_WriteSmallData (0xA4);			//	Default value
    ST7789_WriteSmallData (0xA1);			//	Default value
	/**************** Division line ****************/

	ST7789_WriteCommand(0xE0);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
		ST7789_WriteData(SPI1, data, sizeof(data));
	}

    ST7789_WriteCommand(0xE1);
	{
		uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
		ST7789_WriteData(SPI1, data, sizeof(data));
	}
    ST7789_WriteCommand (ST7789_INVON);		//	Inversion ON
	ST7789_WriteCommand (ST7789_SLPOUT);	//	Out of sleep mode
  	ST7789_WriteCommand (ST7789_NORON);		//	Normal Display on
  	ST7789_WriteCommand (ST7789_DISPON);	//	Main screen turned on	
	
	ST7789_WriteCommand(ST7789_COLMOD);		//	Set color mode
	ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
	
	LL_mDelay(250);
	ST7789_Fill_Color(RED);
	LL_mDelay(250);
	ST7789_Fill_Color(BLUE);
	LL_mDelay(250);
	ST7789_Fill_Color(RED);
	LL_mDelay(200);
	ST7789_WriteString(20, 20, "Steven", Font_7x10, WHITE, BLACK);
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color) {
	
    ST7789_SetAddressWindow(0, 0, 239, 239);
    uint8_t d[] = {color >> 8, color & 0xFF};
    LCD_CS_LOW();
    LCD_DC_HIGH();
    for(uint32_t i = 0; i < 240 * 240; i++) {
        while(!LL_SPI_IsActiveFlag_TXE(SPI1));
        LL_SPI_TransmitData8(SPI1, d[0]);
        while(!LL_SPI_IsActiveFlag_TXE(SPI1));
        LL_SPI_TransmitData8(SPI1, d[1]);
    }
    while(LL_SPI_IsActiveFlag_BSY(SPI1));
    LCD_CS_HIGH();
}

void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x < 0) || (x >= ST7789_WIDTH) ||
		 (y < 0) || (y >= ST7789_HEIGHT))	return;
	
	ST7789_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	ST7789_WriteData(SPI1 , data, sizeof(data));
}

void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
	if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
		 (yEnd < 0) || (yEnd >= ST7789_HEIGHT))	return;
	LCD_CS_LOW();
	uint16_t i, j;
	ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
	for (i = ySta; i <= yEnd; i++)
		for (j = xSta; j <= xEnd; j++) {
			uint8_t data[] = {color >> 8, color & 0xFF};
			ST7789_WriteData(SPI1 , data, sizeof(data));
		}
	LCD_CS_HIGH();
}

/** 
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
	uint32_t i, b, j;
	LCD_CS_LOW();
	ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

	for (i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				uint8_t data[] = {color >> 8, color & 0xFF};
				ST7789_WriteData(SPI1 , data, sizeof(data));
			}
			else {
				uint8_t data[] = {bgcolor >> 8, bgcolor & 0xFF};
				ST7789_WriteData(SPI1 , data, sizeof(data));
			}
		}
	}
	LCD_CS_HIGH();
}

/** 
 * @brief Write a string 
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
	LCD_CS_LOW();
	while (*str) {
		if (x + font.width >= ST7789_WIDTH) {
			x = 0;
			y += font.height;
			if (y + font.height >= ST7789_HEIGHT) {
				break;
			}

			if (*str == ' ') {
				// skip spaces in the beginning of the new line
				str++;
				continue;
			}
		}
		ST7789_WriteChar(x, y, *str, font, color, bgcolor);
		x += font.width;
		str++;
	}
	LCD_CS_HIGH();
}