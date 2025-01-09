/*!
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#include "tlcd_graphic.h"
#include "../lib/util.h"
#include "../os_core.h"
#include "../os_scheduler.h"
#include "../spi/spi.h"
#include "string.h"
#include "tlcd_core.h"

#include <avr/pgmspace.h>
#include <stdint.h>

//----------------------------------------------------------------------------
// Given Functions
//----------------------------------------------------------------------------

/*!
 *  Define a touch area for which the TLCD will send touch events
 *
 *  \param x1 X-coordinate of the starting point
 *  \param y1 Y-coordinate of the starting point
 *  \param x2 X-coordinate of the ending point
 *  \param y2 Y-coordinate of the ending point
 */
void tlcd_defineTouchArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	const uint8_t cmd[] = {ESC_BYTE, A_BYTE, H_BYTE, LOW(x1), HIGH(x1), LOW(y1), HIGH(y1), LOW(x2), HIGH(x2), LOW(y2), HIGH(y2)};
	tlcd_writeCommand(cmd, sizeof(cmd) / sizeof(cmd[0]));
}

/*!
 *  Draw a text at position (x1,y1)
 *
 *  \param x1 X coordinate
 *  \param y1 Y coordinate
 *  \param text Text to draw
 */
void tlcd_drawString(uint16_t x1, uint16_t y1, const char *text)
{
	const uint8_t firstBytes[] = {ESC_BYTE, Z_BYTE, C_BYTE, LOW(x1), HIGH(x1), LOW(y1), HIGH(y1)};
	const uint8_t textLength = strlen(text);
	const uint8_t lastBytes[] = {NUL_BYTE};

	uint8_t len = sizeof(firstBytes) + textLength + sizeof(lastBytes);
	uint8_t header[] = {DC1_BYTE, len};
	uint8_t bcc = INITIAL_BCC_VALUE;

	tlcd_calculateBCC(&bcc, header, sizeof(header));
	tlcd_calculateBCC(&bcc, firstBytes, sizeof(firstBytes));
	tlcd_calculateBCC(&bcc, text, textLength);
	tlcd_calculateBCC(&bcc, lastBytes, sizeof(lastBytes));

	uint8_t retries = 0;

	os_enterCriticalSection();

	do
	{
		spi_writeData(header, sizeof(header));
		spi_writeData((uint8_t *)firstBytes, sizeof(firstBytes));
		spi_writeData((uint8_t *)text, textLength);
		spi_writeData((uint8_t *)lastBytes, sizeof(lastBytes));
		spi_write(bcc);
	} while (spi_read() != ACK && retries++ < TLCD_MAX_RETRIES);

	if (retries >= TLCD_MAX_RETRIES)
	{
		// os_error("no ACK");
	}

	os_leaveCriticalSection();
}

/*!
 *  Draw a text from prog mem at position (x1,y1)
 *
 *  \param x1 X coordinate
 *  \param x1 Y coordinate
 *  \param text Text to draw (address pointing to prog mem)
 */
void tlcd_drawProgString(uint16_t x1, uint16_t y1, const char *text)
{
	const uint8_t firstBytes[] = {ESC_BYTE, Z_BYTE, C_BYTE, LOW(x1), HIGH(x1), LOW(y1), HIGH(y1)};
	const uint8_t textLength = strlen_P(text);
	const uint8_t lastBytes[] = {NUL_BYTE};

	uint8_t len = sizeof(firstBytes) + textLength + sizeof(lastBytes);
	uint8_t header[] = {DC1_BYTE, len};
	uint8_t bcc = INITIAL_BCC_VALUE;

	tlcd_calculateBCC(&bcc, header, sizeof(header));
	tlcd_calculateBCC(&bcc, firstBytes, sizeof(firstBytes));
	tlcd_calculateBCC_ProgMem(&bcc, text, textLength);
	tlcd_calculateBCC(&bcc, lastBytes, sizeof(lastBytes));

	uint8_t retries = 0;

	os_enterCriticalSection();

	do
	{
		spi_writeData(header, sizeof(header));
		spi_writeData((uint8_t *)firstBytes, sizeof(firstBytes));
		spi_writeDataProgMem((uint8_t *)text, textLength);
		spi_writeData((uint8_t *)lastBytes, sizeof(lastBytes));
		spi_write(bcc);
	} while (spi_read() != ACK && retries++ < TLCD_MAX_RETRIES);

	if (retries >= TLCD_MAX_RETRIES)
	{
		// os_error("no ACK");
	}
	os_leaveCriticalSection();
}

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  This function clears the display (fills hole display with background color).
 */
void tlcd_clearDisplay()
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Draw a colored box at given coordinates
 *
 *  \param x1 X-coordinate of the starting point
 *  \param y1 Y-coordinate of the starting point
 *  \param x2 X-coordinate of the ending point
 *  \param y2 Y-coordinate of the ending point
 *  \param fill_color color id of fill color
 */
void tlcd_drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t fill_color)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  This function draws a line in graphic mode from the point (x1,y1) to (x2,y2)
 *
 *  \param x1 X-coordinate of the starting point
 *  \param y1 Y-coordinate of the starting point
 *  \param x2 X-coordinate of the ending point
 *  \param y2 Y-coordinate of the ending point
 */
void tlcd_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  This function draws a point (x1,y1) on the display

 *  \param x1 The position on the x-axis.
 *  \param y1 The position on the y-axis.
 */
void tlcd_drawPoint(uint16_t x1, uint16_t y1)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Change the size of lines being drawn
 *
 *  \param size The new size for the lines
 */
void tlcd_changePenSize(uint8_t size)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Change the color lines are drawn in
 *
 *  \param colorID Index of the color lines should be drawn in
 */
void tlcd_changeDrawColor(uint8_t colorID)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Define the color at a given index. Not all bits are used, refer to data sheet.
 *
 *  \param colorID The color index to change.
 *  \param The color to set.
 */
void tlcd_defineColor(uint8_t colorID, tlcd_color_t color)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Draw a character c at position (x1,y1)
 *
 *  \param c Character to draw
 *  \param x1 X coordinate
 *  \param x1 Y coordinate
 */
void tlcd_drawChar(uint16_t x1, uint16_t y1, char c)
{
	#warning [Praktikum 4] Implement here
}
