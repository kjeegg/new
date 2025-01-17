/*
 *  Modified by FH Aachen
 */

/*************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Peter Fleury <pfleury@gmx.ch>   http://tinyurl.com/peterfleury
File:     $Id: uart.c,v 1.15.2.4 2015/09/05 18:33:32 peter Exp $
Software: AVR-GCC 4.x
Hardware: any AVR with built-in UART, 
License:  GNU General Public License 
          
DESCRIPTION:
    An interrupt is generated when the UART has finished transmitting or
    receiving a byte. The interrupt handling routines use circular buffers
    for buffering received and transmitted data.
    
    The UART_RX_BUFFER_SIZE and UART_TX_BUFFER_SIZE variables define
    the buffer size in bytes. Note that these variables must be a 
    power of 2.
    
USAGE:
    Refere to the header file uart.h for a description of the routines. 
    See also example test_uart.c.

NOTES:
    Based on Atmel Application Note AVR306
                    
LICENSE:
    Copyright (C) 2015 Peter Fleury, GNU General Public License Version 3

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                        
*************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "uart.h"


/*
 *  constants and macros
 */

/* size of RX/TX buffers */
#define UART0_RX_BUFFER_MASK ( UART0_RX_BUFFER_SIZE - 1)
#define UART0_TX_BUFFER_MASK ( UART0_TX_BUFFER_SIZE - 1)

#if ( UART0_RX_BUFFER_SIZE & UART0_RX_BUFFER_MASK )
#error RX0 buffer size is not a power of 2
#endif
#if ( UART0_TX_BUFFER_SIZE & UART0_TX_BUFFER_MASK )
#error TX0 buffer size is not a power of 2
#endif

#define UART1_RX_BUFFER_MASK ( UART1_RX_BUFFER_SIZE - 1)
#define UART1_TX_BUFFER_MASK ( UART1_TX_BUFFER_SIZE - 1)

#if ( UART1_RX_BUFFER_SIZE & UART1_RX_BUFFER_MASK )
#error RX1 buffer size is not a power of 2
#endif
#if ( UART1_TX_BUFFER_SIZE & UART1_TX_BUFFER_MASK )
#error TX1 buffer size is not a power of 2
#endif

#define UART2_RX_BUFFER_MASK ( UART2_RX_BUFFER_SIZE - 1)
#define UART2_TX_BUFFER_MASK ( UART2_TX_BUFFER_SIZE - 1)

#if ( UART2_RX_BUFFER_SIZE & UART2_RX_BUFFER_MASK )
#error RX2 buffer size is not a power of 2
#endif
#if ( UART2_TX_BUFFER_SIZE & UART2_TX_BUFFER_MASK )
#error TX2 buffer size is not a power of 2
#endif

#define UART3_RX_BUFFER_MASK ( UART3_RX_BUFFER_SIZE - 1)
#define UART3_TX_BUFFER_MASK ( UART3_TX_BUFFER_SIZE - 1)

#if ( UART3_RX_BUFFER_SIZE & UART3_RX_BUFFER_MASK )
#error RX2 buffer size is not a power of 2
#endif
#if ( UART3_TX_BUFFER_SIZE & UART3_TX_BUFFER_MASK )
#error TX3 buffer size is not a power of 2
#endif


#if defined(__AVR_AT90S2313__) || defined(__AVR_AT90S4414__) || defined(__AVR_AT90S8515__) || \
    defined(__AVR_AT90S4434__) || defined(__AVR_AT90S8535__) || \
    defined(__AVR_ATmega103__)
 /* old AVR classic or ATmega103 with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect 
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      USR
 #define UART0_CONTROL     UCR
 #define UART0_DATA        UDR  
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_AT90S2333__) || defined(__AVR_AT90S4433__)
 /* old AVR classic with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect 
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_DATA        UDR 
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_AT90PWM216__) || defined(__AVR_AT90PWM316__) 
 /* AT90PWN216/316 with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1 
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega8A__) || \
      defined(__AVR_ATmega16__) || defined(__AVR_ATmega16A__) || \
      defined(__AVR_ATmega32__) || defined(__AVR_ATmega32A__) || \
      defined(__AVR_ATmega323__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RXC_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
 #define UART0_BIT_URSEL   URSEL
#elif defined (__AVR_ATmega8515__) || defined(__AVR_ATmega8535__)
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC 
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
 #define UART0_BIT_URSEL   URSEL
#elif defined(__AVR_ATmega163__)
  /* ATmega163 with one UART */
 #define UART0_RECEIVE_INTERRUPT   UART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  UART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRR
 #define UART0_UBRRH       UBRRHI
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
#elif defined(__AVR_ATmega162__) 
 /* ATmega with two USART */
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   USART0_RXC_vect
 #define UART1_RECEIVE_INTERRUPT   USART1_RXC_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART1_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_URSEL   URSEL0
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
 #define UART1_STATUS      UCSR1A
 #define UART1_CONTROL     UCSR1B
 #define UART1_CONTROLC    UCSR1C
 #define UART1_DATA        UDR1
 #define UART1_UDRIE       UDRIE1
 #define UART1_UBRRL       UBRR1L
 #define UART1_UBRRH       UBRR1H
 #define UART1_BIT_URSEL   URSEL1
 #define UART1_BIT_U2X     U2X1
 #define UART1_BIT_RXCIE   RXCIE1
 #define UART1_BIT_RXEN    RXEN1
 #define UART1_BIT_TXEN    TXEN1
 #define UART1_BIT_UCSZ0   UCSZ10
 #define UART1_BIT_UCSZ1   UCSZ11 
#elif defined(__AVR_ATmega161__)
 /* ATmega with UART */
 #error "AVR ATmega161 currently not supported by this libaray !"
#elif defined(__AVR_ATmega169__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
#elif defined(__AVR_ATmega48__) || defined(__AVR_ATmega48A__) || defined(__AVR_ATmega48P__) || defined(__AVR_ATmega48PA__) || defined(__AVR_ATmega48PB__) || \
      defined(__AVR_ATmega88__) || defined(__AVR_ATmega88A__) || defined(__AVR_ATmega88P__) || defined(__AVR_ATmega88PA__) || defined(__AVR_ATmega88PB__) || \
      defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__)|| defined(__AVR_ATmega168P__)|| defined(__AVR_ATmega168PA__) || defined(__AVR_ATmega168PB__) || \
      defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || \
      defined(__AVR_ATmega3250__) || defined(__AVR_ATmega3290__) ||defined(__AVR_ATmega6450__) || defined(__AVR_ATmega6490__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
#elif defined(__AVR_ATtiny2313__) || defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny4313__)
 /* ATtiny with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
 #define UART0_STATUS      UCSRA
 #define UART0_CONTROL     UCSRB
 #define UART0_CONTROLC    UCSRC
 #define UART0_DATA        UDR
 #define UART0_UDRIE       UDRIE
 #define UART0_UBRRL       UBRRL
 #define UART0_UBRRH       UBRRH
 #define UART0_BIT_U2X     U2X
 #define UART0_BIT_RXCIE   RXCIE
 #define UART0_BIT_RXEN    RXEN
 #define UART0_BIT_TXEN    TXEN
 #define UART0_BIT_UCSZ0   UCSZ0
 #define UART0_BIT_UCSZ1   UCSZ1
#elif defined(__AVR_ATmega329__) || defined(__AVR_ATmega649__) || defined(__AVR_ATmega3290__) || defined(__AVR_ATmega6490__) ||\
      defined(__AVR_ATmega169A__) || defined(__AVR_ATmega169PA__) || \
      defined(__AVR_ATmega329A__) || defined(__AVR_ATmega329PA__) || defined(__AVR_ATmega3290A__) || defined(__AVR_ATmega3290PA__) || \
      defined(__AVR_ATmega649A__) || defined(__AVR_ATmega649P__) || defined(__AVR_ATmega6490A__) || defined(__AVR_ATmega6490P__) || \
      defined(__AVR_ATmega165__) || defined(__AVR_ATmega325__) || defined(__AVR_ATmega645__) || defined(__AVR_ATmega3250__) || defined(__AVR_ATmega6450__) || \
      defined(__AVR_ATmega165A__) || defined(__AVR_ATmega165PA__) || \
      defined(__AVR_ATmega325A__) || defined(__AVR_ATmega325PA__) || defined(__AVR_ATmega3250A__) || defined(__AVR_ATmega3250PA__) ||\
      defined(__AVR_ATmega645A__) || defined(__AVR_ATmega645PA__) || defined(__AVR_ATmega6450A__) || defined(__AVR_ATmega6450PA__) || \
      defined(__AVR_ATmega644__)
 /* ATmega with one USART */
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
#elif defined(__AVR_ATmega64__) || defined(__AVR_ATmega128__) || defined(__AVR_ATmega128A__) ||\
      defined(__AVR_ATmega640__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2561__) || \
      defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644P__) ||  \
      defined(__AVR_ATmega164A__) || defined(__AVR_ATmega164PA__) || defined(__AVR_ATmega324A__) || defined(__AVR_ATmega324PA__) || \
      defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644PA__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||\
      defined(__AVR_ATtiny1634__)
 /* ATmega with two USART */
 #define ATMEGA_USART1
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART1_RECEIVE_INTERRUPT   USART1_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART1_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C  
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01 
 #define UART1_STATUS      UCSR1A
 #define UART1_CONTROL     UCSR1B
 #define UART1_CONTROLC    UCSR1C  
 #define UART1_DATA        UDR1
 #define UART1_UDRIE       UDRIE1
 #define UART1_UBRRL       UBRR1L
 #define UART1_UBRRH       UBRR1H
 #define UART1_BIT_U2X     U2X1
 #define UART1_BIT_RXCIE   RXCIE1
 #define UART1_BIT_RXEN    RXEN1
 #define UART1_BIT_TXEN    TXEN1
 #define UART1_BIT_UCSZ0   UCSZ10
 #define UART1_BIT_UCSZ1   UCSZ11
#elif defined(__AVR_ATmega2560__)
 /* ATmega with four USART - Extension by David Thoennessen - proTEC-Vision Automation GmbH */
 #define ATMEGA_USART1
 #define ATMEGA_USART2
 #define ATMEGA_USART3
 #define UART0_RECEIVE_INTERRUPT   USART0_RX_vect
 #define UART1_RECEIVE_INTERRUPT   USART1_RX_vect
 #define UART2_RECEIVE_INTERRUPT   USART2_RX_vect
 #define UART3_RECEIVE_INTERRUPT   USART3_RX_vect 
 #define UART0_TRANSMIT_INTERRUPT  USART0_UDRE_vect
 #define UART1_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART2_TRANSMIT_INTERRUPT  USART2_UDRE_vect
 #define UART3_TRANSMIT_INTERRUPT  USART3_UDRE_vect
 #define UART0_STATUS      UCSR0A
 #define UART0_CONTROL     UCSR0B
 #define UART0_CONTROLC    UCSR0C
 #define UART0_DATA        UDR0
 #define UART0_UDRIE       UDRIE0
 #define UART0_UBRRL       UBRR0L
 #define UART0_UBRRH       UBRR0H
 #define UART0_BIT_U2X     U2X0
 #define UART0_BIT_RXCIE   RXCIE0
 #define UART0_BIT_RXEN    RXEN0
 #define UART0_BIT_TXEN    TXEN0
 #define UART0_BIT_UCSZ0   UCSZ00
 #define UART0_BIT_UCSZ1   UCSZ01
 #define UART1_STATUS      UCSR1A
 #define UART1_CONTROL     UCSR1B
 #define UART1_CONTROLC    UCSR1C
 #define UART1_DATA        UDR1
 #define UART1_UDRIE       UDRIE1
 #define UART1_UBRRL       UBRR1L
 #define UART1_UBRRH       UBRR1H
 #define UART1_BIT_U2X     U2X1
 #define UART1_BIT_RXCIE   RXCIE1
 #define UART1_BIT_RXEN    RXEN1
 #define UART1_BIT_TXEN    TXEN1
 #define UART1_BIT_UCSZ0   UCSZ10
 #define UART1_BIT_UCSZ1   UCSZ11
 #define UART2_STATUS      UCSR2A
 #define UART2_CONTROL     UCSR2B
 #define UART2_CONTROLC    UCSR2C
 #define UART2_DATA        UDR2
 #define UART2_UDRIE       UDRIE2
 #define UART2_UBRRL       UBRR2L
 #define UART2_UBRRH       UBRR2H
 #define UART2_BIT_U2X     U2X2
 #define UART2_BIT_RXCIE   RXCIE2
 #define UART2_BIT_RXEN    RXEN2
 #define UART2_BIT_TXEN    TXEN2
 #define UART2_BIT_UCSZ0   UCSZ20
 #define UART2_BIT_UCSZ1   UCSZ21
 #define UART3_STATUS      UCSR3A
 #define UART3_CONTROL     UCSR3B
 #define UART3_CONTROLC    UCSR3C
 #define UART3_DATA        UDR3
 #define UART3_UDRIE       UDRIE3
 #define UART3_UBRRL       UBRR3L
 #define UART3_UBRRH       UBRR3H
 #define UART3_BIT_U2X     U2X3
 #define UART3_BIT_RXCIE   RXCIE3
 #define UART3_BIT_RXEN    RXEN3
 #define UART3_BIT_TXEN    TXEN3
 #define UART3_BIT_UCSZ0   UCSZ30
 #define UART3_BIT_UCSZ1   UCSZ31
#elif defined(__AVR_ATmega8U2__) || defined(__AVR_ATmega16U2__) || defined(__AVR_ATmega32U2__) || \
      defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__) || \
      defined(__AVR_AT90USB82__) || defined(__AVR_AT90USB162__) || \
      defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB647__) || defined(__AVR_AT90USB1287__)
 #define UART0_RECEIVE_INTERRUPT   USART1_RX_vect
 #define UART0_TRANSMIT_INTERRUPT  USART1_UDRE_vect
 #define UART0_STATUS      UCSR1A
 #define UART0_CONTROL     UCSR1B
 #define UART0_CONTROLC    UCSR1C
 #define UART0_DATA        UDR1
 #define UART0_UDRIE       UDRIE1
 #define UART0_UBRRL       UBRR1L
 #define UART0_UBRRH       UBRR1H
 #define UART0_BIT_U2X     U2X1
 #define UART0_BIT_RXCIE   RXCIE1
 #define UART0_BIT_RXEN    RXEN1
 #define UART0_BIT_TXEN    TXEN1
 #define UART0_BIT_UCSZ0   UCSZ10
 #define UART0_BIT_UCSZ1   UCSZ11
#else
 #error "no UART definition for MCU available"
#endif



/*
 *  module global variables
 */
static volatile unsigned char UART0_TxBuf[UART0_TX_BUFFER_SIZE];
static volatile unsigned char UART0_RxBuf[UART0_RX_BUFFER_SIZE];
static volatile unsigned char UART0_TxHead;
static volatile unsigned char UART0_TxTail;
static volatile unsigned char UART0_RxHead;
static volatile unsigned char UART0_RxTail;
static volatile unsigned char UART0_LastRxError;

#if defined( ATMEGA_USART1 )
static volatile unsigned char UART1_TxBuf[UART1_TX_BUFFER_SIZE];
static volatile unsigned char UART1_RxBuf[UART1_RX_BUFFER_SIZE];
static volatile unsigned char UART1_TxHead;
static volatile unsigned char UART1_TxTail;
static volatile unsigned char UART1_RxHead;
static volatile unsigned char UART1_RxTail;
static volatile unsigned char UART1_LastRxError;
#endif

#if defined( ATMEGA_USART2 )
static volatile unsigned char UART2_TxBuf[UART2_TX_BUFFER_SIZE];
static volatile unsigned char UART2_RxBuf[UART2_RX_BUFFER_SIZE];
static volatile unsigned char UART2_TxHead;
static volatile unsigned char UART2_TxTail;
static volatile unsigned char UART2_RxHead;
static volatile unsigned char UART2_RxTail;
static volatile unsigned char UART2_LastRxError;
#endif

#if defined( ATMEGA_USART3 )
static volatile unsigned char UART3_TxBuf[UART3_TX_BUFFER_SIZE];
static volatile unsigned char UART3_RxBuf[UART3_RX_BUFFER_SIZE];
static volatile unsigned char UART3_TxHead;
static volatile unsigned char UART3_TxTail;
static volatile unsigned char UART3_RxHead;
static volatile unsigned char UART3_RxTail;
static volatile unsigned char UART3_LastRxError;
#endif


//! USART 0 =======================================================================================

ISR (UART0_RECEIVE_INTERRUPT)	
/*************************************************************************
Function: UART Receive Complete interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    /* read UART status register and UART data register */
    usr  = UART0_STATUS;
    data = UART0_DATA;
    
    /* get FEn (Frame Error) DORn (Data OverRun) UPEn (USART Parity Error) bits */
#if defined(FE) && defined(DOR) && defined(UPE)
    lastRxError = usr & (_BV(FE)|_BV(DOR)|_BV(UPE) );
#elif defined(FE0) && defined(DOR0) && defined(UPE0)
    lastRxError = usr & (_BV(FE0)|_BV(DOR0)|_BV(UPE0) );
#elif defined(FE1) && defined(DOR1) && defined(UPE1)
    lastRxError = usr & (_BV(FE1)|_BV(DOR1)|_BV(UPE1) );
#elif defined(FE) && defined(DOR)
    lastRxError = usr & (_BV(FE)|_BV(DOR) );
#endif

    /* calculate buffer index */ 
    tmphead = ( UART0_RxHead + 1) & UART0_RX_BUFFER_MASK;
    
    if ( tmphead == UART0_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART0_RxHead = tmphead;
        /* store received data in buffer */
        UART0_RxBuf[tmphead] = data;
    }
    UART0_LastRxError |= lastRxError;   
}


ISR (UART0_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART Data Register Empty interrupt
Purpose:  called when the UART is ready to transmit the next byte
**************************************************************************/
{
    unsigned char tmptail;

    
    if ( UART0_TxHead != UART0_TxTail) {
        /* calculate and store new buffer index */
        tmptail = (UART0_TxTail + 1) & UART0_TX_BUFFER_MASK;
        UART0_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART0_DATA = UART0_TxBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UART0_CONTROL &= ~_BV(UART0_UDRIE);
    }
}


/*************************************************************************
Function: uart0_init()
Purpose:  initialize UART and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart0_init(unsigned int baudrate)
{
    UART0_TxHead = 0;
    UART0_TxTail = 0;
    UART0_RxHead = 0;
    UART0_RxTail = 0;

#ifdef UART_TEST
#ifndef UART0_BIT_U2X
#warning "UART0_BIT_U2X not defined"
#endif
#ifndef UART0_UBRRH
#warning "UART0_UBRRH not defined"
#endif
#ifndef UART0_CONTROLC
#warning "UART0_CONTROLC not defined"
#endif
#if defined(URSEL) || defined(URSEL0)
#ifndef UART0_BIT_URSEL
#warning "UART0_BIT_URSEL not defined"
#endif
#endif
#endif

    /* Set baud rate */
    if ( baudrate & 0x8000 )
    {
        #if UART0_BIT_U2X
        UART0_STATUS = (1<<UART0_BIT_U2X);  //Enable 2x speed 
        #endif
    } 
    #if defined(UART0_UBRRH)
    UART0_UBRRH = (unsigned char)((baudrate>>8)&0x80) ;
    #endif    
    UART0_UBRRL = (unsigned char) (baudrate&0x00FF);
      
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UART0_CONTROL = _BV(UART0_BIT_RXCIE)|(1<<UART0_BIT_RXEN)|(1<<UART0_BIT_TXEN);
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */
    #ifdef UART0_CONTROLC
    #ifdef UART0_BIT_URSEL
    UART0_CONTROLC = (1<<UART0_BIT_URSEL)|(1<<UART0_BIT_UCSZ1)|(1<<UART0_BIT_UCSZ0);
    #else
    UART0_CONTROLC = (1<<UART0_BIT_UCSZ1)|(1<<UART0_BIT_UCSZ0);
    #endif 
    #endif

}/* uart0_init */


/*************************************************************************
Function: uart0_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart0_getc(void)
{    
    unsigned char tmptail;
    unsigned char data;
    unsigned char lastRxError;


    if ( UART0_RxHead == UART0_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (UART0_RxTail + 1) & UART0_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = UART0_RxBuf[tmptail];
    lastRxError = UART0_LastRxError;
    
    /* store buffer index */
    UART0_RxTail = tmptail; 
    
    UART0_LastRxError = 0;
    return (lastRxError << 8) + data;

}/* uart0_getc */


/*************************************************************************
Function: uart0_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart0_putc(unsigned char data)
{
    unsigned char tmphead;

    
    tmphead  = (UART0_TxHead + 1) & UART0_TX_BUFFER_MASK;
    
    while ( tmphead == UART0_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART0_TxBuf[tmphead] = data;
    UART0_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART0_CONTROL    |= _BV(UART0_UDRIE);

}/* uart0_putc */


/*************************************************************************
Function: uart0_puts()
Purpose:  transmit string to UART
Input:    string to be transmitted
Returns:  none          
**************************************************************************/
void uart0_puts(const char *s )
{
    while (*s) 
      uart0_putc(*s++);

}/* uart0_puts */


/*************************************************************************
Function: uart0_puts_p()
Purpose:  transmit string from program memory to UART
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart0_puts_p(const char *progmem_s )
{
    register char c;
    
    while ( (c = pgm_read_byte(progmem_s++)) ) 
      uart0_putc(c);

}/* uart0_puts_p */

//! Extensions by David Thoennessen - proTEC-Vision Automation GmbH ===========
#ifndef cbi
#define cbi(x, b) (x &= ~(1 << (b)))
#endif

inline uint16_t BUFFER_FILLING(uint16_t head, uint16_t tail, uint16_t size)
{
	if (head >= tail) { return head - tail; }
	return size - tail + head;
}

/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart0_getrxcount()
{
	return BUFFER_FILLING(UART0_RxHead, UART0_RxTail, UART0_RX_BUFFER_SIZE);
}

/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart0_gettxcount()
{
	return BUFFER_FILLING(UART0_TxHead, UART0_TxTail, UART0_TX_BUFFER_SIZE);
}

/*
 * Disables the RX/TX ports to not provide the connected device with energy
 */
void uart0_disable()
{
	cbi(UART0_CONTROL, UART0_BIT_RXEN);
	cbi(UART0_CONTROL, UART0_BIT_TXEN);
}
//! ===========================================================================

//! USART 1 =======================================================================================
/*
 * these functions are only for ATmegas with two USART
 */
#if defined( ATMEGA_USART1 )

ISR(UART1_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART1 Receive Complete interrupt
Purpose:  called when the UART1 has received a character
**************************************************************************/
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    /* read UART status register and UART data register */ 
    usr  = UART1_STATUS;
    data = UART1_DATA;
    
    /* get FEn (Frame Error) DORn (Data OverRun) UPEn (USART Parity Error) bits */
    lastRxError = usr & (_BV(FE1)|_BV(DOR1)|_BV(UPE1) );
            
    /* calculate buffer index */ 
    tmphead = ( UART1_RxHead + 1) & UART1_RX_BUFFER_MASK;
    
    if ( tmphead == UART1_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART1_RxHead = tmphead;
        /* store received data in buffer */
        UART1_RxBuf[tmphead] = data;
    }
    UART1_LastRxError |= lastRxError;   
}


ISR(UART1_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART1 Data Register Empty interrupt
Purpose:  called when the UART1 is ready to transmit the next byte
**************************************************************************/
{
    unsigned char tmptail;

    
    if ( UART1_TxHead != UART1_TxTail) {
        /* calculate and store new buffer index */
        tmptail = (UART1_TxTail + 1) & UART1_TX_BUFFER_MASK;
        UART1_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART1_DATA = UART1_TxBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UART1_CONTROL &= ~_BV(UART1_UDRIE);
    }
}


/*************************************************************************
Function: uart1_init()
Purpose:  initialize UART1 and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart1_init(unsigned int baudrate)
{
    UART1_TxHead = 0;
    UART1_TxTail = 0;
    UART1_RxHead = 0;
    UART1_RxTail = 0;

#ifdef UART_TEST
#ifndef UART1_BIT_U2X
#warning "UART1_BIT_U2X not defined"
#endif
#ifndef UART1_UBRRH
#warning "UART1_UBRRH not defined"
#endif
#ifndef UART1_CONTROLC
#warning "UART1_CONTROLC not defined"
#endif
#if defined(URSEL) || defined(URSEL1)
#ifndef UART1_BIT_URSEL
#warning "UART1_BIT_URSEL not defined"
#endif
#endif
#endif

    /* Set baud rate */
    if ( baudrate & 0x8000 ) 
    {
        #if UART1_BIT_U2X
    	UART1_STATUS = (1<<UART1_BIT_U2X);  //Enable 2x speed 
        #endif
    }
    UART1_UBRRH = (unsigned char)((baudrate>>8)&0x80) ;
    UART1_UBRRL = (unsigned char) baudrate;
        
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UART1_CONTROL = _BV(UART1_BIT_RXCIE)|(1<<UART1_BIT_RXEN)|(1<<UART1_BIT_TXEN);    
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */   
    #ifdef UART1_BIT_URSEL
    UART1_CONTROLC = (1<<UART1_BIT_URSEL)|(1<<UART1_BIT_UCSZ1)|(1<<UART1_BIT_UCSZ0);
    #else
    UART1_CONTROLC = (1<<UART1_BIT_UCSZ1)|(1<<UART1_BIT_UCSZ0);
    #endif 
    
}/* uart_init */


/*************************************************************************
Function: uart1_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart1_getc(void)
{    
    unsigned char tmptail;
    unsigned int  data;
    unsigned char lastRxError;


    if ( UART1_RxHead == UART1_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (UART1_RxTail + 1) & UART1_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = UART1_RxBuf[tmptail];
    lastRxError = UART1_LastRxError;
    
    /* store buffer index */
    UART1_RxTail = tmptail; 
    
    UART1_LastRxError = 0;
    return (lastRxError << 8) + data;

}/* uart1_getc */


/*************************************************************************
Function: uart1_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart1_putc(unsigned char data)
{
    unsigned char tmphead;

    
    tmphead  = (UART1_TxHead + 1) & UART1_TX_BUFFER_MASK;
    
    while ( tmphead == UART1_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART1_TxBuf[tmphead] = data;
    UART1_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART1_CONTROL    |= _BV(UART1_UDRIE);

}/* uart1_putc */


/*************************************************************************
Function: uart1_puts()
Purpose:  transmit string to UART1
Input:    string to be transmitted
Returns:  none          
**************************************************************************/
void uart1_puts(const char *s )
{
    while (*s) 
      uart1_putc(*s++);

}/* uart1_puts */


/*************************************************************************
Function: uart1_puts_p()
Purpose:  transmit string from program memory to UART1
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart1_puts_p(const char *progmem_s )
{
    register char c;
    
    while ( (c = pgm_read_byte(progmem_s++)) ) 
      uart1_putc(c);

}/* uart1_puts_p */


//! Extensions by David Thoennessen - proTEC-Vision Automation GmbH ===========
/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart1_getrxcount()
{
	return BUFFER_FILLING(UART1_RxHead, UART1_RxTail, UART1_RX_BUFFER_SIZE);
}

/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart1_gettxcount()
{
	return BUFFER_FILLING(UART1_TxHead, UART1_TxTail, UART1_TX_BUFFER_SIZE);
}

/*
 * Disables the RX/TX ports to not provide the connected device with energy
 */
void uart1_disable()
{
	cbi(UART1_CONTROL, UART1_BIT_RXEN);
	cbi(UART1_CONTROL, UART1_BIT_TXEN);
}
//! ===========================================================================
#endif

//! USART 2 =======================================================================================
/*
 * these functions are only for ATmegas with three or more USART
 */
#if defined( ATMEGA_USART2 )

ISR(UART2_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART2 Receive Complete interrupt
Purpose:  called when the UART2 has received a character
**************************************************************************/
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    /* read UART status register and UART data register */ 
    usr  = UART2_STATUS;
    data = UART2_DATA;
    
    /* get FEn (Frame Error) DORn (Data OverRun) UPEn (USART Parity Error) bits */
    lastRxError = usr & (_BV(FE1)|_BV(DOR1)|_BV(UPE1) );
            
    /* calculate buffer index */ 
    tmphead = ( UART2_RxHead + 1) & UART2_RX_BUFFER_MASK;
    
    if ( tmphead == UART2_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART2_RxHead = tmphead;
        /* store received data in buffer */
        UART2_RxBuf[tmphead] = data;
    }
    UART2_LastRxError |= lastRxError;   
}

/* -- Modifications by FH Aachen -- */
void uart2_flush_blocking()
{
    unsigned char tmptail;

    while ( UART2_TxHead != UART2_TxTail)
    {
        while (!( UCSR2A & (1<<UDRE2)));
        /* calculate and store new buffer index */
        tmptail = (UART2_TxTail + 1) & UART2_TX_BUFFER_MASK;
        UART2_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART2_DATA = UART2_TxBuf[tmptail];  /* start transmission */
    }
}
/* --------------------------------*/

ISR(UART2_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART2 Data Register Empty interrupt
Purpose:  called when the UART2 is ready to transmit the next byte
**************************************************************************/
{
    unsigned char tmptail;

    
    if ( UART2_TxHead != UART2_TxTail) {
        /* calculate and store new buffer index */
        tmptail = (UART2_TxTail + 1) & UART2_TX_BUFFER_MASK;
        UART2_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART2_DATA = UART2_TxBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UART2_CONTROL &= ~_BV(UART2_UDRIE);
    }
}


/*************************************************************************
Function: uart2_init()
Purpose:  initialize UART2 and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart2_init(unsigned int baudrate)
{
    UART2_TxHead = 0;
    UART2_TxTail = 0;
    UART2_RxHead = 0;
    UART2_RxTail = 0;

#ifdef UART_TEST
#ifndef UART2_BIT_U2X
#warning "UART2_BIT_U2X not defined"
#endif
#ifndef UART2_UBRRH
#warning "UART2_UBRRH not defined"
#endif
#ifndef UART2_CONTROLC
#warning "UART2_CONTROLC not defined"
#endif
#if defined(URSEL) || defined(URSEL1)
#ifndef UART2_BIT_URSEL
#warning "UART2_BIT_URSEL not defined"
#endif
#endif
#endif

    /* Set baud rate */
    if ( baudrate & 0x8000 ) 
    {
        #if UART2_BIT_U2X
    	UART2_STATUS = (1<<UART2_BIT_U2X);  //Enable 2x speed 
        #endif
    }
    UART2_UBRRH = (unsigned char)((baudrate>>8)&0x80) ;
    UART2_UBRRL = (unsigned char) baudrate;
        
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UART2_CONTROL = _BV(UART2_BIT_RXCIE)|(1<<UART2_BIT_RXEN)|(1<<UART2_BIT_TXEN);    
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */   
    #ifdef UART2_BIT_URSEL
    UART2_CONTROLC = (1<<UART2_BIT_URSEL)|(1<<UART2_BIT_UCSZ1)|(1<<UART2_BIT_UCSZ0);
    #else
    UART2_CONTROLC = (1<<UART2_BIT_UCSZ1)|(1<<UART2_BIT_UCSZ0);
    #endif 
    
}/* uart2_init */


/*************************************************************************
Function: uart2_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart2_getc(void)
{    
    unsigned char tmptail;
    unsigned int  data;
    unsigned char lastRxError;


    if ( UART2_RxHead == UART2_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (UART2_RxTail + 1) & UART2_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = UART2_RxBuf[tmptail];
    lastRxError = UART2_LastRxError;
    
    /* store buffer index */
    UART2_RxTail = tmptail; 
    
    UART2_LastRxError = 0;
    return (lastRxError << 8) + data;

}/* uart2_getc */


/*************************************************************************
Function: uart2_putc()
Purpose:  write byte to ringbuffer for transmitting via UART2
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart2_putc(unsigned char data)
{
    unsigned char tmphead;

    
    tmphead  = (UART2_TxHead + 1) & UART2_TX_BUFFER_MASK;
    
    while ( tmphead == UART2_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART2_TxBuf[tmphead] = data;
    UART2_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART2_CONTROL    |= _BV(UART2_UDRIE);

}/* uart2_putc */


/*************************************************************************
Function: uart2_puts()
Purpose:  transmit string to UART2
Input:    string to be transmitted
Returns:  none          
**************************************************************************/
void uart2_puts(const char *s )
{
    while (*s) 
      uart2_putc(*s++);

}/* uart2_puts */


/*************************************************************************
Function: uart2_puts_p()
Purpose:  transmit string from program memory to UART2
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart2_puts_p(const char *progmem_s )
{
    register char c;
    
    while ( (c = pgm_read_byte(progmem_s++)) ) 
      uart2_putc(c);

}/* uart2_puts_p */


//! Extensions by David Thoennessen - proTEC-Vision Automation GmbH ===========
/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart2_getrxcount()
{
	return BUFFER_FILLING(UART2_RxHead, UART2_RxTail, UART2_RX_BUFFER_SIZE);
}

/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart2_gettxcount()
{
	return BUFFER_FILLING(UART2_TxHead, UART2_TxTail, UART2_TX_BUFFER_SIZE);
}

/*
 * Disables the RX/TX ports to not provide the connected device with energy
 */
void uart2_disable()
{
	cbi(UART2_CONTROL, UART2_BIT_RXEN);
	cbi(UART2_CONTROL, UART2_BIT_TXEN);
}
//! ===========================================================================
#endif

//! USART 3 =======================================================================================
/*
 * these functions are only for ATmegas with four USART
 */
#if defined( ATMEGA_USART3 )

ISR(UART3_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART3 Receive Complete interrupt
Purpose:  called when the UART3 has received a character
**************************************************************************/
{
    unsigned char tmphead;
    unsigned char data;
    unsigned char usr;
    unsigned char lastRxError;
 
 
    /* read UART status register and UART data register */ 
    usr  = UART3_STATUS;
    data = UART3_DATA;
    
    /* get FEn (Frame Error) DORn (Data OverRun) UPEn (USART Parity Error) bits */
    lastRxError = usr & (_BV(FE1)|_BV(DOR1)|_BV(UPE1) );
            
    /* calculate buffer index */ 
    tmphead = ( UART3_RxHead + 1) & UART3_RX_BUFFER_MASK;
    
    if ( tmphead == UART3_RxTail ) {
        /* error: receive buffer overflow */
        lastRxError = UART_BUFFER_OVERFLOW >> 8;
    }else{
        /* store new index */
        UART3_RxHead = tmphead;
        /* store received data in buffer */
        UART3_RxBuf[tmphead] = data;
    }
    UART3_LastRxError |= lastRxError;   
}


ISR(UART3_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART3 Data Register Empty interrupt
Purpose:  called when the UART3 is ready to transmit the next byte
**************************************************************************/
{
    unsigned char tmptail;

    
    if ( UART3_TxHead != UART3_TxTail) {
        /* calculate and store new buffer index */
        tmptail = (UART3_TxTail + 1) & UART3_TX_BUFFER_MASK;
        UART3_TxTail = tmptail;
        /* get one byte from buffer and write it to UART */
        UART3_DATA = UART3_TxBuf[tmptail];  /* start transmission */
    }else{
        /* tx buffer empty, disable UDRE interrupt */
        UART3_CONTROL &= ~_BV(UART3_UDRIE);
    }
}


/*************************************************************************
Function: uart3_init()
Purpose:  initialize UART3 and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart3_init(unsigned int baudrate)
{
    UART3_TxHead = 0;
    UART3_TxTail = 0;
    UART3_RxHead = 0;
    UART3_RxTail = 0;

#ifdef UART_TEST
#ifndef UART3_BIT_U2X
#warning "UART3_BIT_U2X not defined"
#endif
#ifndef UART3_UBRRH
#warning "UART3_UBRRH not defined"
#endif
#ifndef UART3_CONTROLC
#warning "UART3_CONTROLC not defined"
#endif
#if defined(URSEL) || defined(URSEL1)
#ifndef UART3_BIT_URSEL
#warning "UART3_BIT_URSEL not defined"
#endif
#endif
#endif

    /* Set baud rate */
    if ( baudrate & 0x8000 ) 
    {
        #if UART3_BIT_U2X
    	UART3_STATUS = (1<<UART3_BIT_U2X);  //Enable 2x speed 
        #endif
    }
    UART3_UBRRH = (unsigned char)((baudrate>>8)&0x80) ;
    UART3_UBRRL = (unsigned char) baudrate;
        
    /* Enable USART receiver and transmitter and receive complete interrupt */
    UART3_CONTROL = _BV(UART3_BIT_RXCIE)|(1<<UART3_BIT_RXEN)|(1<<UART3_BIT_TXEN);    
    
    /* Set frame format: asynchronous, 8data, no parity, 1stop bit */   
    #ifdef UART3_BIT_URSEL
    UART3_CONTROLC = (1<<UART3_BIT_URSEL)|(1<<UART3_BIT_UCSZ1)|(1<<UART3_BIT_UCSZ0);
    #else
    UART3_CONTROLC = (1<<UART3_BIT_UCSZ1)|(1<<UART3_BIT_UCSZ0);
    #endif 
    
}/* uart3_init */


/*************************************************************************
Function: uart3_getc()
Purpose:  return byte from ringbuffer  
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
unsigned int uart3_getc(void)
{    
    unsigned char tmptail;
    unsigned int  data;
    unsigned char lastRxError;


    if ( UART3_RxHead == UART3_RxTail ) {
        return UART_NO_DATA;   /* no data available */
    }
    
    /* calculate buffer index */
    tmptail = (UART3_RxTail + 1) & UART3_RX_BUFFER_MASK;
    
    /* get data from receive buffer */
    data = UART3_RxBuf[tmptail];
    lastRxError = UART3_LastRxError;
    
    /* store buffer index */
    UART3_RxTail = tmptail; 
    
    UART3_LastRxError = 0;
    return (lastRxError << 8) + data;

}/* uart3_getc */


/*************************************************************************
Function: uart3_putc()
Purpose:  write byte to ringbuffer for transmitting via UART3
Input:    byte to be transmitted
Returns:  none          
**************************************************************************/
void uart3_putc(unsigned char data)
{
    unsigned char tmphead;

    
    tmphead  = (UART3_TxHead + 1) & UART3_TX_BUFFER_MASK;
    
    while ( tmphead == UART3_TxTail ){
        ;/* wait for free space in buffer */
    }
    
    UART3_TxBuf[tmphead] = data;
    UART3_TxHead = tmphead;

    /* enable UDRE interrupt */
    UART3_CONTROL    |= _BV(UART3_UDRIE);

}/* uart3_putc */


/*************************************************************************
Function: uart3_puts()
Purpose:  transmit string to UART3
Input:    string to be transmitted
Returns:  none          
**************************************************************************/
void uart3_puts(const char *s )
{
    while (*s) 
      uart3_putc(*s++);

}/* uart3_puts */


/*************************************************************************
Function: uart3_puts_p()
Purpose:  transmit string from program memory to UART3
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart3_puts_p(const char *progmem_s )
{
    register char c;
    
    while ( (c = pgm_read_byte(progmem_s++)) ) 
      uart3_putc(c);

}/* uart3_puts_p */


//! Extensions by David Thoennessen - proTEC-Vision Automation GmbH ===========
/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart3_getrxcount()
{
	return BUFFER_FILLING(UART3_RxHead, UART3_RxTail, UART3_RX_BUFFER_SIZE);
}

/*
 * Returns current filling of the buffer in byte
 */
uint16_t uart3_gettxcount()
{
	return BUFFER_FILLING(UART3_TxHead, UART3_TxTail, UART3_TX_BUFFER_SIZE);
}

/*
 * Disables the RX/TX ports to not provide the connected device with energy
 */
void uart3_disable()
{
	cbi(UART3_CONTROL, UART3_BIT_RXEN);
	cbi(UART3_CONTROL, UART3_BIT_TXEN);
}
//! ===========================================================================

#endif
