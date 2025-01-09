/*!
 *  \brief Layer built on top of UART where frames get assembled.
 *
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#include "serialAdapter.h"
#include "../lib/lcd.h"
#include "../lib/util.h"
#include "../os_core.h"
#include "../lib/terminal.h"
#include "../os_scheduler.h"
#include "rfAdapter.h"
#include "xbee.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <string.h>

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------

//! Timeout for receiving frames
#define SERIAL_ADAPTER_READ_TIMEOUT_MS ((time_t)500)

//----------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------

//! Calculates a checksum of given data
void serialAdapter_calculateChecksum(checksum_t *checksum, void *data, uint8_t length);

//! Calculates a checksum of the frame
void serialAdapter_calculateFrameChecksum(checksum_t *checksum, frame_t *frame);

//! Returns true if timestamp + timeoutMs is a timestamp in the past
bool serialAdapter_hasTimeout(time_t timestamp, time_t timeoutMs);

//----------------------------------------------------------------------------
// Given functions
//----------------------------------------------------------------------------

/*!
 *  Checks if a given timestamp has timed out
 *
 *  \param timestamp Timestamp to check
 *  \param timeoutMs Timeout in milliseconds
 *  \return True if timestamp + timeoutMs is a timestamp in the past
 */
bool serialAdapter_hasTimeout(time_t timestamp, time_t timeoutMs)
{
	return (getSystemTime_ms() - timestamp >= timeoutMs);
}

/*!
 *  Blocks process until at least one byte is available to be read
 */
void serialAdapter_waitForAnyByte()
{
	while (xbee_getNumberOfBytesReceived() == 0)
	{
		os_yield();
	}
}

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  Initializes the serialAdapter and their dependencies
 */
void serialAdapter_init(void)
{
	xbee_init();
}

/*!
 *  Sends a frame with given innerFrame
 *
 *  \param destAddr where to send the frame to
 *  \param length how many bytes the innerFrame has
 *  \param innerFrame buffer as payload of the frame
 */
void serialAdapter_writeFrame(address_t destAddr, inner_frame_length_t length, inner_frame_t *innerFrame)
{
	

	frame_t frame;
	frame.header.startFlag = serialAdapter_startFlag;
	frame.header.srcAddr = serialAdapter_address;
	frame.header.destAddr = destAddr;
	frame.header.length = length;
	// Kopiere innerFrame-Daten in frame.innerFrame
	memcpy(&frame.innerFrame, innerFrame, length);
	checksum_t checksum = INITIAL_CHECKSUM_VALUE;
	serialAdapter_calculateFrameChecksum(&checksum, &frame);

	

	// Header senden
	xbee_writeData(&frame.header.startFlag, sizeof(frame.header.startFlag));
	xbee_writeData(&frame.header.srcAddr, sizeof(frame.header.srcAddr));
	xbee_writeData(&frame.header.destAddr, sizeof(frame.header.destAddr));
	xbee_writeData(&frame.header.length, sizeof(frame.header.length));
	

	// InnerFrame senden
	xbee_writeData(innerFrame, length);

	// Footer (Checksum) senden
	frame.footer.checksum = checksum;
	DEBUG("Frame sent with Command: %x", frame.innerFrame.command);
	xbee_writeData(&frame.footer.checksum, sizeof(frame.footer.checksum));


}


/*!
 *  Blocks process until byteCount bytes are available to be read.
 *
 *  \param byteCount Count of bytes that need to arrive so that the function will unblock
 *  \param frameTimestamp Start time of the first byte arrived from which the timeout will be calculated on
 *  \return False when it times out.
 */
bool serialAdapter_waitForData(uint8_t byteCount, time_t frameTimestamp)
{
	 while (xbee_getNumberOfBytesReceived() < byteCount)
	 {
		 if (serialAdapter_hasTimeout(frameTimestamp, SERIAL_ADAPTER_READ_TIMEOUT_MS))
		 {
			 // Timeout
			 
			 return false;
		 }
		 os_yield();
	 }
	 
	 return true;
}

/*!
 *  Reads incoming data and processes it. Needs to be called periodically.
 *  Don't read from UART in any other process while this is running.
 */
void serialAdapter_worker()
{
		time_t timestamp = getSystemTime_ms();
		
		if(!serialAdapter_waitForData(2, timestamp)){
			return;
		}
		
		uint8_t low, high;
		
		xbee_readBuffer(&low, 1);
		if(low != (uint8_t) serialAdapter_startFlag){
			return;
		}
		xbee_readBuffer(&high, 1);
		if(high != (serialAdapter_startFlag >> 8)){
			return;
		}
			
		
		frame_t frame;
		frame.header.startFlag = serialAdapter_startFlag;
		time_t startTime = getSystemTime_ms();
		
		
		
		uint8_t status ;
		
		

		status = xbee_readBuffer((uint8_t *)&frame.header.srcAddr, sizeof(frame.header.srcAddr));
		if (status != XBEE_SUCCESS) { return; }

		status = xbee_readBuffer((uint8_t *)&frame.header.destAddr, sizeof(frame.header.destAddr));
		if (status != XBEE_SUCCESS) { return; }

		status = xbee_readBuffer((uint8_t *)&frame.header.length, sizeof(frame.header.length));
		if (status != XBEE_SUCCESS) { return; }

		if (frame.header.length > COMM_MAX_INNER_FRAME_LENGTH)
		{
			// Ungültige Länge
			return;
		}

		
		uint8_t totalNeeded = frame.header.length + COMM_FOOTER_LENGTH;
		if (!serialAdapter_waitForData(totalNeeded, startTime))
		{
			// Timeout beim Lesen von InnerFrame+Footer
			return;
		}

		status = xbee_readBuffer((uint8_t *)&frame.innerFrame, frame.header.length);
		if (status != XBEE_SUCCESS) { return; }

		status = xbee_readBuffer((uint8_t *)&frame.footer.checksum, sizeof(frame.footer.checksum));
		if (status != XBEE_SUCCESS) { return; }

		checksum_t calcCheck = INITIAL_CHECKSUM_VALUE;
		serialAdapter_calculateFrameChecksum(&calcCheck, &frame);
		if (calcCheck != frame.footer.checksum)
		{	
			
			// Prüfsumme stimmt nicht
			return;
		}

		if (frame.header.destAddr != serialAdapter_address && frame.header.destAddr != ADDRESS_BROADCAST)
		{
			// Nicht für uns
			return;
		}
		
		//printFrame(&frame," ");
		
		serialAdapter_processFrame(&frame);

		

		// rest 
	
}

/*!
 *  Calculates a checksum of given data
 *
 *  \param checksum pointer to a checksum that will be updated
 *  \param data buffer on which the checksum will be calculated
 *  \param length size of the given buffer
 */
void serialAdapter_calculateChecksum(checksum_t *checksum, void *data, uint8_t length)
{
	uint8_t *ptr = (uint8_t *)data;
	for (uint8_t i = 0; i < length; i++)
	{
		*checksum ^= ptr[i];
	}
}

/*!
 *  Calculates a checksum of the frame
 *
 *  \param checksum pointer to a checksum that will be updated
 *  \param frame data on which the checksum will be calculated
 */
void serialAdapter_calculateFrameChecksum(checksum_t *checksum, frame_t *frame)
{
	 *checksum = INITIAL_CHECKSUM_VALUE;
	 // Header einbeziehen
	 serialAdapter_calculateChecksum(checksum, &frame->header, sizeof(frame->header));
	 // innerFrame einbeziehen (command + payload)
	 serialAdapter_calculateChecksum(checksum, &frame->innerFrame, frame->header.length);
	
}


