/*
 * Board.h
 *
 * Created: 10.01.2025 20:06:48
 *  Author: alram
 */ 


#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>

// AVR Ports:
// B0: pin #3 reserved connector
// B1: pin #1 reserved connector
// B2: LED error
// B3: SPI: MOSI
// B4: SPI: MISO
// B5: SPI: CLK
// B6: Push button: Channel
// B7: Push button: Reset counter
// C0: XN297L CSN (chip select)
// C1: XN297L CE (enable transmit/receive)
// C2: LED blue
// C3: LED green
// C4: I2C SDA
// C5: I2C SCL
// C6: Reset AVR
// D0: pin #3 debug-connector (UART RX)
// D1: pin #4 debug-connector (UART TX)
// D2: XN297 IRQ
// D3: pin #9 SPI-connector
// D4: LED red
// D5: Push button: Start
// D6: pin #6 reserved connector
// D7: pin #8 SPI-connector, pin #5 reserved connector

#define console_write(format, args...)   printf_P(PSTR(format) , ## args)

#define BOARD_LED_ERROR_ON  (PORTB |=   1<<PORTB2)
#define BOARD_LED_ERROR_OFF (PORTB &= ~(1<<PORTB2))

#define BOARD_LED_CHANNEL_GREEN_ON  (PORTC |=   1<<PORTC3)
#define BOARD_LED_CHANNEL_GREEN_OFF (PORTC &= ~(1<<PORTC3))

#define BOARD_LED_CHANNEL_RED_ON  (PORTD |=   1<<PORTD4)
#define BOARD_LED_CHANNEL_RED_OFF (PORTD &= ~(1<<PORTD4))

#define BOARD_LED_CHANNEL_BLUE_ON  (PORTC |=   1<<PORTC2)
#define BOARD_LED_CHANNEL_BLUE_OFF (PORTC &= ~(1<<PORTC2))

#define BOARD_SPI_XN297_CS_SELECT   (PORTC &= ~(1<<PORTC0))
#define BOARD_SPI_XN297_CS_DESELECT (PORTC |=   1<<PORTC0)

#define BOARD_SPI_XN297_CE_INACTIVE (PORTC &= ~(1<<PORTC1))
#define BOARD_SPI_XN297_CE_ACTIVE   (PORTC |=   1<<PORTC1)

#define BOARD_XN297_IRQ_ENABLE (EIMSK |= (1<<INT0))
#define BOARD_XN297_IRQ_DISABLE (EIMSK &= ~(1<<INT0))

// active if value != zero; not debounced!
#define BOARD_BUTTON_STATE_CHANNEL (PINB&(1<<PINB6))
#define BOARD_BUTTON_STATE_RESET (PINB&(1<<PINB7))
#define BOARD_BUTTON_STATE_START (PIND&(1<<PIND5))

/************************************************************************/
/* init the board:                                                      */
/* - IO pins                                                            */
/************************************************************************/
void board_init();

unsigned char board_spi_put( const unsigned char value );
void board_usart_write_char(const const char c);

void board_test_loop();

void board_turn_off() ;

#endif /* BOARD_H_ */