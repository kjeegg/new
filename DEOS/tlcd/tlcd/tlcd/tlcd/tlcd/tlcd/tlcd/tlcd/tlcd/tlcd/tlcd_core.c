/*!
 *  \author   Fachbereich 5 - FH Aachen
 *  \date     2024
 *  \version  1.0
 */

#include "tlcd_core.h"
#include "../lib/atmega2560constants.h"
#include "../lib/lcd.h"
#include "../lib/util.h"
#include "../os_core.h"
#include "../os_scheduler.h"
#include "../spi/spi.h"
#include "tlcd_graphic.h"

#include <util/delay.h>

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

bool tlcd_initialized = false;

//----------------------------------------------------------------------------
// Given functions
//----------------------------------------------------------------------------

/*!
 *  This function requests the sending buffer from the TLCD. Should only be called, if SBUF pin is low (or through polling).
 */
void tlcd_requestData()
{
	uint8_t bytesToSend[] = {DC2_BYTE, 0x01, S_BYTE};

	uint8_t bcc = INITIAL_BCC_VALUE;
	tlcd_calculateBCC(&bcc, bytesToSend, sizeof(bytesToSend));

	uint8_t retries = 0;

	do
	{
		spi_writeData(bytesToSend, sizeof(bytesToSend));
		spi_write(bcc);
	} while (spi_read() != ACK && retries++ < TLCD_MAX_RETRIES);

	if (retries >= TLCD_MAX_RETRIES)
	{
		// os_error("no ACK");
	}
}

//----------------------------------------------------------------------------
// Your Homework
//----------------------------------------------------------------------------

/*!
 *  Initializes the TLCD
 */
void tlcd_init()
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Returns true if the TLCD has been initialized
 */
bool tlcd_isInitialized()
{
	#warning [Praktikum 4] Implement here
}

/*!
 * Sends a command to the TLCD. Header and checksum will be added automatically.
 *
 * \param cmd pointer to the command data buffer
 * \param len length of the command data buffer
 */
void tlcd_writeCommand(const void *cmd, uint8_t len)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Calculates the BCC of a given data buffer and adds it to the given BCC value through mutating it.
 *
 *  \param bcc pointer to the BCC value that will be mutated
 *  \param data pointer to the data buffer
 *  \param len length of the data buffer
 */
void tlcd_calculateBCC(uint8_t *bcc, const void *data, uint8_t len)
{
	#warning [Praktikum 4] Implement here
}

/*!
 *  Calculates the BCC of a given data buffer in program memory and adds it to the given BCC value through mutating it.
 *
 *  \param bcc pointer to the BCC value that will be mutated
 *  \param data pointer to the data buffer in program memory
 *  \param len length of the data buffer
 */
void tlcd_calculateBCC_ProgMem(uint8_t *bcc, const void *data, uint8_t len)
{
	#warning [Praktikum 4] Implement here
}
