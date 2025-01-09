#ifndef UART_H
#define UART_H

/*
 *  Modified by FH Aachen.
 */

/************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>  http://tinyurl.com/peterfleury
File:     $Id: uart.h,v 1.13 2015/01/11 13:53:25 peter Exp $
Software: AVR-GCC 4.x, AVR Libc 1.4 or higher
Hardware: any AVR with built-in UART/USART
Usage:    see Doxygen manual

LICENSE:
    Copyright (C) 2015 Peter Fleury, GNU General Public License Version 3

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
************************************************************************/

/** 
 *  @file
 *  @defgroup pfleury_uart UART Library <uart.h>
 *  @code #include <uart.h> @endcode
 * 
 *  @brief Interrupt UART library using the built-in UART with transmit and receive circular buffers. 
 *
 *  This library can be used to transmit and receive data through the built in UART. 
 *
 *  An interrupt is generated when the UART has finished transmitting or
 *  receiving a byte. The interrupt handling routines use circular buffers
 *  for buffering received and transmitted data.
 *
 *  The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE constants define
 *  the size of the circular buffers in bytes. Note that these constants must be a power of 2.
 *  You may need to adapt these constants to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn -DUART_TX_BUFFER_SIZE=nn to your Makefile.
 *
 *  @note Based on Atmel Application Note AVR306
 *  @author Peter Fleury pfleury@gmx.ch  http://tinyurl.com/peterfleury
 *  @copyright (C) 2015 Peter Fleury, GNU General Public License Version 3
 */
 

#include <avr/pgmspace.h>

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 405
#error "This library requires AVR-GCC 4.5 or later, update to newer AVR-GCC compiler !"
#endif

/* -- Modifications by FH Aachen -- */
void uart2_flush_blocking();
/* --------------------------------*/


/**@{*/


/*
** constants and macros
*/


/** @brief  UART Baudrate Expression
 *  @param  xtalCpu  system clock in Mhz, e.g. 4000000UL for 4Mhz          
 *  @param  baudRate baudrate in bps, e.g. 1200, 2400, 9600     
 */
#define UART_BAUD_SELECT(baudRate,xtalCpu)  (((xtalCpu) + 8UL * (baudRate)) / (16UL * (baudRate)) -1UL)

/** @brief  UART Baudrate Expression for ATmega double speed mode
 *  @param  xtalCpu  system clock in Mhz, e.g. 4000000UL for 4Mhz           
 *  @param  baudRate baudrate in bps, e.g. 1200, 2400, 9600     
 */
#define UART_BAUD_SELECT_DOUBLE_SPEED(baudRate,xtalCpu) ( ((((xtalCpu) + 4UL * (baudRate)) / (8UL * (baudRate)) -1UL)) | 0x8000)

/** @brief  Size of the UART0 circular receive buffer, must be power of 2, and <= 256
 * 
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART0_RX_BUFFER_SIZE
#define UART0_RX_BUFFER_SIZE 0
#endif

/** @brief  Size of the UART0 circular transmit buffer, must be power of 2, and <= 256
 *
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_TX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART0_TX_BUFFER_SIZE
#define UART0_TX_BUFFER_SIZE 0
#endif

/** @brief  Size of the UART1 circular receive buffer, must be power of 2, and <= 256
 * 
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART1_RX_BUFFER_SIZE
#define UART1_RX_BUFFER_SIZE 256
#endif

/** @brief  Size of the UART1 circular transmit buffer, must be power of 2, and <= 256
 *
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_TX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART1_TX_BUFFER_SIZE
#define UART1_TX_BUFFER_SIZE 64
#endif

/** @brief  Size of the UART2 circular receive buffer, must be power of 2, and <= 256
 * 
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART2_RX_BUFFER_SIZE
#define UART2_RX_BUFFER_SIZE 16
#endif

/** @brief  Size of the UART2 circular transmit buffer, must be power of 2, and <= 256
 *
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_TX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART2_TX_BUFFER_SIZE
#define UART2_TX_BUFFER_SIZE 256
#endif

/** @brief  Size of the UART3 circular receive buffer, must be power of 2, and <= 256
 * 
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_RX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART3_RX_BUFFER_SIZE
#define UART3_RX_BUFFER_SIZE 64
#endif

/** @brief  Size of the UART3 circular transmit buffer, must be power of 2, and <= 256
 *
 *  You may need to adapt this constant to your target and your application by adding 
 *  CDEFS += -DUART_TX_BUFFER_SIZE=nn to your Makefile.
 */
#ifndef UART3_TX_BUFFER_SIZE
#define UART3_TX_BUFFER_SIZE 64
#endif

/* test if the size of the circular buffers fits into SRAM */
#if ( (UART0_RX_BUFFER_SIZE + UART0_TX_BUFFER_SIZE + \
       UART1_RX_BUFFER_SIZE + UART1_TX_BUFFER_SIZE + \
       UART2_RX_BUFFER_SIZE + UART2_TX_BUFFER_SIZE + \
       UART3_RX_BUFFER_SIZE + UART3_TX_BUFFER_SIZE) >= (RAMEND-0x60 ) )
#error "size of UART buffers is larger than size of SRAM"
#endif

/* 
** high byte error return code of uart_getc()
*/
#define UART_FRAME_ERROR      0x1000              /**< @brief Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0800              /**< @brief Overrun condition by UART   */
#define UART_PARITY_ERROR     0x0400              /**< @brief Parity Error by UART        */ 
#define UART_BUFFER_OVERFLOW  0x0200              /**< @brief receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /**< @brief no receive data available   */


/*
** function prototypes
*/

/**
   @brief   Initialize UART and set baudrate 
   @param   baudrate Specify baudrate using macro UART_BAUD_SELECT()
   @return  none
*/
extern void uart0_init(unsigned int baudrate);


/**
 *  @brief   Get received byte from ringbuffer
 *
 * Returns in the lower byte the received character and in the 
 * higher byte the last receive error.
 * UART_NO_DATA is returned when no data is available.
 *
 *  @return  lower byte:  received byte from ringbuffer
 *  @return  higher byte: last receive status
 *           - \b 0 successfully received data from UART
 *           - \b UART_NO_DATA           
 *             <br>no receive data available
 *           - \b UART_BUFFER_OVERFLOW   
 *             <br>Receive ringbuffer overflow.
 *             We are not reading the receive buffer fast enough, 
 *             one or more received character have been dropped 
 *           - \b UART_OVERRUN_ERROR     
 *             <br>Overrun condition by UART.
 *             A character already present in the UART UDR register was 
 *             not read by the interrupt handler before the next character arrived,
 *             one or more received characters have been dropped.
 *           - \b UART_FRAME_ERROR       
 *             <br>Framing Error by UART
 */
extern unsigned int uart0_getc(void);


/**
 *  @brief   Put byte to ringbuffer for transmitting via UART
 *  @param   data byte to be transmitted
 *  @return  none
 */
extern void uart0_putc(unsigned char data);


/**
 *  @brief   Put string to ringbuffer for transmitting via UART
 *
 *  The string is buffered by the uart library in a circular buffer
 *  and one character at a time is transmitted to the UART using interrupts.
 *  Blocks if it can not write the whole string into the circular buffer.
 * 
 *  @param   s string to be transmitted
 *  @return  none
 */
extern void uart0_puts(const char *s );


/**
 * @brief    Put string from program memory to ringbuffer for transmitting via UART.
 *
 * The string is buffered by the uart library in a circular buffer
 * and one character at a time is transmitted to the UART using interrupts.
 * Blocks if it can not write the whole string into the circular buffer.
 *
 * @param    s program memory string to be transmitted
 * @return   none
 * @see      uart_puts_P
 */
extern void uart0_puts_p(const char *s );

/**
 * @brief    Macro to automatically put a string constant into program memory
 */
#define uart0_puts_P(__s)       uart0_puts_p(PSTR(__s))



/** @brief  Initialize USART1 (only available on selected ATmegas) @see uart_init */
extern void uart1_init(unsigned int baudrate);
/** @brief  Get received byte of USART1 from ringbuffer. (only available on selected ATmega) @see uart_getc */
extern unsigned int uart1_getc(void);
/** @brief  Put byte to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_putc */
extern void uart1_putc(unsigned char data);
/** @brief  Put string to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts */
extern void uart1_puts(const char *s );
/** @brief  Put string from program memory to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts_p */
extern void uart1_puts_p(const char *s );
/** @brief  Macro to automatically put a string constant into program memory */
#define uart1_puts_P(__s)       uart1_puts_p(PSTR(__s))

/** @brief  Initialize USART2 (only available on selected ATmegas) @see uart_init */
extern void uart2_init(unsigned int baudrate);
/** @brief  Get received byte of USART2 from ringbuffer. (only available on selected ATmega) @see uart_getc */
extern unsigned int uart2_getc(void);
/** @brief  Put byte to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see uart_putc */
extern void uart2_putc(unsigned char data);
/** @brief  Put string to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see uart_puts */
extern void uart2_puts(const char *s );
/** @brief  Put string from program memory to ringbuffer for transmitting via USART2 (only available on selected ATmega) @see uart_puts_p */
extern void uart2_puts_p(const char *s );
/** @brief  Macro to automatically put a string constant into program memory */
#define uart2_puts_P(__s)       uart2_puts_p(PSTR(__s))

/** @brief  Initialize USART3 (only available on selected ATmegas) @see uart_init */
extern void uart3_init(unsigned int baudrate);
/** @brief  Get received byte of USART3 from ringbuffer. (only available on selected ATmega) @see uart_getc */
extern unsigned int uart3_getc(void);
/** @brief  Put byte to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see uart_putc */
extern void uart3_putc(unsigned char data);
/** @brief  Put string to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see uart_puts */
extern void uart3_puts(const char *s );
/** @brief  Put string from program memory to ringbuffer for transmitting via USART3 (only available on selected ATmega) @see uart_puts_p */
extern void uart3_puts_p(const char *s );
/** @brief  Macro to automatically put a string constant into program memory */
#define uart3_puts_P(__s)       uart3_puts_p(PSTR(__s))

/**@}*/

//! Extensions by David Thoennessen - proTEC-Vision Automation GmbH

//! Returns current filling of the buffer in byte
extern uint16_t uart0_getrxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart1_getrxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart2_getrxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart3_getrxcount();

//! Returns current filling of the buffer in byte
extern uint16_t uart0_gettxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart1_gettxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart2_gettxcount();
//! Returns current filling of the buffer in byte
extern uint16_t uart3_gettxcount();

//! Disables the RX/TX ports to not provide the connected device with energy
extern void uart0_disable();
//! Disables the RX/TX ports to not provide the connected device with energy
extern void uart1_disable();
//! Disables the RX/TX ports to not provide the connected device with energy
extern void uart2_disable();
//! Disables the RX/TX ports to not provide the connected device with energy
extern void uart3_disable();


#endif // UART_H 

