/*!
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#ifndef TLCD_GRAPHIC_H_
#define TLCD_GRAPHIC_H_

#include <stdint.h>

//! Struct to define a color
typedef struct TLCD_Color
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} tlcd_color_t;

//! Defines a free touch area. Events that occur in this area are caught
void tlcd_defineTouchArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

//! Draws a line on the display with given start and endpoint
void tlcd_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

//! Draws a point to a given location on the display
void tlcd_drawPoint(uint16_t x1, uint16_t y1);

//! Changes the size of the pen
void tlcd_changePenSize(uint8_t size);

//! Changes the color of the pen
void tlcd_changeDrawColor(uint8_t color);

//! Draws a rectangle on the display
void tlcd_drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);

//! Draws a circle on the display
void tlcd_defineColor(uint8_t colorID, tlcd_color_t color);

//! Draws a character on the display
void tlcd_drawChar(uint16_t x1, uint16_t y1, char c);

//! Draws a string on the display
void tlcd_drawString(uint16_t x1, uint16_t y1, const char *text);

//! Draws a string on the display from program memory
void tlcd_drawProgString(uint16_t x1, uint16_t y1, const char *text);

//! Clears the display
void tlcd_clearDisplay();

#endif /* TLCD_GRAPHIC_H_ */