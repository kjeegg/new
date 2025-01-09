/*!
 *  \brief Layer forwarding the underlying UART library.
 *
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#include "xbee.h"
#include "../lib/uart.h"
#include "../lib/terminal.h"
#include "../os_scheduler.h"
#include "rfAdapter.h"

#include <avr/interrupt.h>

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  Initializes the XBee
 */
void xbee_init()
{
	uart1_init(UART_BAUD_SELECT(38400, F_CPU));
	
}

/*!
 *  Transmits one byte to the XBee
 *
 *  \param byte one byte that will be sent through UART
 */
void xbee_write(uint8_t byte)
{
	uart1_putc(byte);
}

/*!
 *  Receives one byte from the XBee
 *
 *  \param byte Reference parameter where the read byte will be written to
 *  \return Error code or XBEE_SUCCESS. When XBEE_BUFFER_INCONSISTENCY gets returned, `byte` still gets updated
 */
uint8_t xbee_read(uint8_t *byte)
{
	unsigned int uart_data = uart1_getc();
	unsigned int status = uart_data & 0xFF00;
	uint8_t data = (uint8_t)(uart_data & 0x00FF);

	if (status == 0)
	{
		*byte = data;
		
		return XBEE_SUCCESS;
	}
	else
	{
		
		if (status & UART_FRAME_ERROR)
		{
			return XBEE_READ_ERROR;
		}
		else if (status & (UART_OVERRUN_ERROR | UART_BUFFER_OVERFLOW))
		{
			*byte = data;
			return XBEE_BUFFER_INCONSISTENCY;
		}
		else if (status & UART_NO_DATA)
		{
			return XBEE_DATA_MISSING;
		}
		else
		{
			return XBEE_READ_ERROR;
		}
	}
}

/*!
 *  Transmits the given data to the XBee
 *
 *  \param data buffer with will be sent through UART
 *  \param length size of the buffer
 */
void xbee_writeData(void *data, uint8_t length)
{
	
	uint8_t *ptr = (uint8_t *)data;
	for (uint8_t i = 0; i < length; i++)
	{
		xbee_write(ptr[i]);
	}
	
}

/*!
 *	Returns current filling of the buffer in byte
 *
 *  \return count of bytes that can be received through `xbee_read`
 */
uint16_t xbee_getNumberOfBytesReceived()
{
	uint16_t count = uart1_getrxcount();
	
	return count;
}

/*!
 *  Receives `length` bytes and writes them to `buffer`. Make sure there are enough bytes to be read
 *
 *	\param message_buffer Buffer where to store received bytes
 *  \param buffer_size Amount of bytes that need to be received
 *  \return error Code if there is any. If it's not XBEE_SUCCESS, you shouldn't use the result
 */
uint8_t xbee_readBuffer(uint8_t *buffer, uint8_t length)
{
	
	if (xbee_getNumberOfBytesReceived() < length)
	{
		
		return XBEE_DATA_MISSING;
	}

	for (uint8_t i = 0; i < length; i++)
	{
		uint8_t result = xbee_read(&buffer[i]);
		if (result != XBEE_SUCCESS)
		{
			
			return result;
		}
	}

	
	return XBEE_SUCCESS;
}

