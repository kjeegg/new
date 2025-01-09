/*!
 *  \brief Layer built on top of serialAdapter where commands are defined.
 *
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#include "rfAdapter.h"
#include "../lib/lcd.h"
#include "../os_core.h"
#include "../lib/terminal.h"
#include <string.h>


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Flag that indicates whether the adapter has been initialized
bool rfAdapter_initialized = false;

//! Start-Flag that announces a new frame
start_flag_t serialAdapter_startFlag = 0x5246; // "RF"

//! Configuration what address this microcontroller has
address_t serialAdapter_address = ADDRESS(1, 0);

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

void rfAdapter_receiveSetLed(cmd_setLed_t *);
void rfAdapter_receiveToggleLed();
void rfAdapter_receiveLcdGoto(cmd_lcdGoto_t *);
void rfAdapter_receiveLcdPrint(cmd_lcdPrint_t *);
void rfAdapter_receiveLcdClear();

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  Initializes the rfAdapter and their dependencies
 */
void rfAdapter_init()
{
	serialAdapter_init();
	// PB7 als Ausgang für LED
	DDRB |= (1 << PB7);
	rfAdapter_initialized = true;
}

/*!
 * Check if adapter has been initialized
 *
 * \return True if the communication has been initialized
 */
uint8_t rfAdapter_isInitialized()
{
	return (rfAdapter_initialized ? 1 : 0);
}

/*!
 *  Main task of adapter
 */
void rfAdapter_worker()
{
	serialAdapter_worker();
}

/*!
 *  Is called on command frame receive
 *
 *  \param frame Received frame
 */
void serialAdapter_processFrame(frame_t *frame)
{
	// Prüfen, ob length mindestens 1 Byte für command umfasst
	if (frame->header.length < 1)
	{
		// Ungültiges Frame
		return;
	}

	command_t cmd = frame->innerFrame.command;
	DEBUG("Frame with Command: %x", cmd);
	switch (cmd)
	{
		case CMD_SET_LED:
		{
			if (frame->header.length == sizeof(command_t) + sizeof(cmd_setLed_t))
			{
				cmd_setLed_t *data = (cmd_setLed_t *)(frame->innerFrame.payload);
				rfAdapter_receiveSetLed(data);
			}
			break;
		}
		case CMD_TOGGLE_LED:
		{
			// Keine Nutzdaten außer command_t erwartet
			if (frame->header.length == sizeof(command_t))
			{
				rfAdapter_receiveToggleLed();
			}
			break;
		}
		case CMD_LCD_CLEAR:
		{
			// Keine Nutzdaten außer command_t erwartet
			if (frame->header.length == sizeof(command_t))
			{
				rfAdapter_receiveLcdClear();
			}
			break;
		}
		case CMD_LCD_GOTO:
		{
			if (frame->header.length == sizeof(command_t) + sizeof(cmd_lcdGoto_t))
			{
				cmd_lcdGoto_t *data = (cmd_lcdGoto_t *)(frame->innerFrame.payload);
				rfAdapter_receiveLcdGoto(data);
			}
			break;
		}
		case CMD_LCD_PRINT:
		{
			// mind. command_t + length + payload
			// Struktur: [command][length][message...]
			if (frame->header.length >= sizeof(command_t) + sizeof(uint8_t))
			{
				cmd_lcdPrint_t *data = (cmd_lcdPrint_t *)frame->innerFrame.payload;
				uint8_t length = data->length;
				if (frame->header.length == sizeof(command_t) + sizeof(uint8_t) + length)
				{
					// Prüfen, dass length <= 32
					if (length <= 32)
					{
						rfAdapter_receiveLcdPrint(data);
					}
				}
			}
			break;
		}
		case CMD_SENSOR_DATA:
		{
			// Hier könnte man sensordaten verarbeiten, falls gefordert.
			
			break;
		}
		default:
		// Unbekannter Befehl
		break;
	}
}

/*!
 *  Handler that's called when command CMD_SET_LED was received
 *
 *  \param data Payload of received frame
 */
void rfAdapter_receiveSetLed(cmd_setLed_t *data)
{
	if (data->enable)
	{
		PORTB |= (1 << PB7); // LED an
	}
	else
	{
		PORTB &= ~(1 << PB7); // LED aus
	}
}

/*!
 *  Handler that's called when command CMD_TOGGLE_LED was received
 */
void rfAdapter_receiveToggleLed()
{
	// LED Zustand umschalten
	PORTB ^= (1 << PB7);
}

/*!
 *  Handler that's called when command CMD_LCD_CLEAR was received
 */
void rfAdapter_receiveLcdClear()
{
	lcd_clear();
}

/*!
 *  Handler that's called when command CMD_LCD_GOTO was received
 *
 *  \param data Payload of received frame
 */
void rfAdapter_receiveLcdGoto(cmd_lcdGoto_t *data)
{
	lcd_goto(data->x, data->y);
}

/*!
 *  Handler that's called when command CMD_LCD_PRINT was received
 *
 *  \param data Payload of received frame
 */
void rfAdapter_receiveLcdPrint(cmd_lcdPrint_t *data)
{
	// Wir müssen sicherstellen, dass ein Terminierungszeichen angehängt wird
	char buffer[33]; // max 32 Zeichen + '\0'
	memcpy(buffer, data->message, data->length);
	buffer[data->length] = '\0';
	lcd_writeString(buffer);
}

/*!
 *  Sends a frame with command CMD_SET_LED
 *
 *  \param destAddr Where to send the frame
 *  \param enable Whether the receiver should enable or disable their led
 */
void rfAdapter_sendSetLed(address_t destAddr, bool enable)
{
	inner_frame_t innerFrame;
	innerFrame.command = CMD_SET_LED;
	cmd_setLed_t payload;
	payload.enable = (enable ? 1 : 0);
	memcpy(innerFrame.payload, &payload, sizeof(payload));

	inner_frame_length_t length = sizeof(innerFrame.command) + sizeof(payload);
	serialAdapter_writeFrame(destAddr, length, &innerFrame);
}

/*!
 *  Sends a frame with command CMD_TOGGLE_LED
 *
 *  \param destAddr Where to send the frame
 */
void rfAdapter_sendToggleLed(address_t destAddr)
{
	inner_frame_t innerFrame;
	innerFrame.command = CMD_TOGGLE_LED;
	inner_frame_length_t length = sizeof(innerFrame.command);
	serialAdapter_writeFrame(destAddr, length, &innerFrame);
}

/*!
 *  Sends a frame with command CMD_LCD_CLEAR
 *
 *  \param destAddr Where to send the frame
 */
void rfAdapter_sendLcdClear(address_t destAddr)
{
	inner_frame_t innerFrame;
	innerFrame.command = CMD_LCD_CLEAR;
	inner_frame_length_t length = sizeof(innerFrame.command);
	serialAdapter_writeFrame(destAddr, length, &innerFrame);
}

/*!
 *  Sends a frame with command CMD_LCD_GOTO
 *
 *  \param destAddr Where to send the frame
 *  \param x Which column should be selected by the receiver
 *  \param y Which row should be selected by the receiver
 */
void rfAdapter_sendLcdGoto(address_t destAddr, uint8_t x, uint8_t y)
{
	inner_frame_t innerFrame;
	innerFrame.command = CMD_LCD_GOTO;
	cmd_lcdGoto_t payload;
	payload.x = x;
	payload.y = y;
	memcpy(innerFrame.payload, &payload, sizeof(payload));
	inner_frame_length_t length = sizeof(innerFrame.command) + sizeof(payload);
	serialAdapter_writeFrame(destAddr, length, &innerFrame);
}

/*!
 *  Sends a frame with command CMD_LCD_PRINT
 *
 *  \param destAddr Where to send the frame
 *  \param message Which message should be printed on receiver side
 */
void rfAdapter_sendLcdPrint(address_t destAddr, const char *message)
{
	inner_frame_t innerFrame;
	innerFrame.command = CMD_LCD_PRINT;
	uint8_t msgLength = (uint8_t)strlen(message);
	if (msgLength > 32) msgLength = 32; // Begrenzen, falls nötig

	cmd_lcdPrint_t payload;
	payload.length = msgLength;
	memcpy(payload.message, message, msgLength);

	// Daten ins innerFrame kopieren
	memcpy(innerFrame.payload, &payload, sizeof(payload.length) + msgLength);

	inner_frame_length_t length = sizeof(innerFrame.command) + sizeof(payload.length) + msgLength;
	serialAdapter_writeFrame(destAddr, length, &innerFrame);
}

/*!
 *  Sends a frame with command CMD_LCD_PRINT
 *
 *  \param destAddr Where to send the frame
 *  \param message Which message should be printed on receiver side as address to program memory. Use PSTR for creating strings on program memory
 */
void rfAdapter_sendLcdPrintProcMem(address_t destAddr, const char *message)
{
	// Nachricht aus dem Programmspeicher lesen
	char buffer[33];
	uint8_t i = 0;
	for (; i < 32; i++)
	{
		char c = pgm_read_byte(&message[i]);
		if (c == '\0') break;
		buffer[i] = c;
	}
	buffer[i] = '\0';

	rfAdapter_sendLcdPrint(destAddr, buffer);
}

void rfAdapter_sendSensorData(address_t destAddr, sensor_type_t sensorType, sensor_parameter_type_t paramType, float value) {
	inner_frame_t innerFrame;
	cmd_sensorData_t data;

	// ????????? ????????? ???????
	data.sensor = sensorType;
	data.paramType = paramType;
	data.param.fValue = value;

	innerFrame.command = CMD_SENSOR_DATA;
	memcpy(innerFrame.payload, &data, sizeof(data));

	// ?????????? ?????? ????? serialAdapter
	serialAdapter_writeFrame(destAddr, sizeof(innerFrame.command) + sizeof(data), &innerFrame);
}
