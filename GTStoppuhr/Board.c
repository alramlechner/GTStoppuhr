/*
 * Board.c
 *
 * Created: 10.01.2025 20:17:45
 *  Author: alram
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "Board.h"
#include "driver/oled_ssd1306.h"
#include "xn297l.h"

static void board_spi_init(void);
static void board_usart_init(void);
static void board_i2c_init(void);
static void board_timer_init(void);

static int stdout_handler( char c, FILE *stream )
{
	//usart_cons_write_char(c);
	board_usart_write_char(c);
	return 0;
}

static FILE consstdout = FDEV_SETUP_STREAM( stdout_handler, NULL, _FDEV_SETUP_WRITE );

void board_init() {
	// IO direction (1==output)
	// B0: pin #3 reserved connector
	// B1: pin #1 reserved connector
	// B2: LED error
	// B3: SPI: MOSI
	// B4: SPI: MISO
	// B5: SPI: CLK
	// B6: Push button: Channel
	// B7: Push button: Reset counter
	DDRB  = (0<<PORTB0)|(0<<PORTB1)|(1<<PORTB2)|(1<<PORTB3)|(0<<PORTB4)|(1<<PORTB5)|(0<<PORTB6)|(0<<PORTB7);
	// enable pull-up for buttons & SPI CLK:
	PORTB = (0<<PORTB0)|(0<<PORTB1)|(0<<PORTB2)|(0<<PORTB3)|(0<<PORTB4)|(0<<PORTB5)|(1<<PORTB6)|(1<<PORTB7);
	
	// C0: XN297L SPI CSN (chip select)
	// C1: XN297L CE (enable transmit/receive)
	// C2: LED blue
	// C3: LED green
	// C4: I2C SDA
	// C5: I2C SCL
	// C6: Reset AVR
	DDRC  = (1<<PORTC0)|(1<<PORTC1)|(1<<PORTC2)|(1<<PORTC3)|(0<<PORTC4)|(0<<PORTC5)|(0<<PORTC6);
	// by default pull-up SPI CSN (disable the chip)
	PORTC = (1<<PORTC0)|(0<<PORTC1)|(0<<PORTC2)|(0<<PORTC3)|(0<<PORTC4)|(0<<PORTC5)|(0<<PORTC6);
	
	// D0: pin #3 debug-connector (UART RX)
	// D1: pin #4 debug-connector (UART TX)
	// D2: XN297 IRQ
	// D3: pin #9 SPI-connector
	// D4: LED red
	// D5: Push button: Start
	// D6: pin #6 reserved connector
	// D7: pin #8 SPI-connector, pin #5 reserved connector
	DDRD  = (0<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(1<<PORTD4)|(0<<PORTD5)|(0<<PORTD6)|(0<<PORTD7);
	// enable pull-up for buttons:
	PORTD = (0<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(0<<PORTD4)|(1<<PORTD5)|(0<<PORTD6)|(0<<PORTD7);
	
	stdout = &consstdout;
	board_spi_init();
	board_usart_init();
	board_i2c_init();
	board_timer_init();

	// enable interrupts for buttons:
	PCMSK2 |= 1<<PCINT21;
	PCMSK0 |= (1<<PCINT6) | (1<<PCINT7);
	PCICR |= (1<<PCIE2) | (1<<PCIE0);
}

void board_timer_init(void) {
}

static void board_i2c_init(void) {
	// replaces i2c_init(); set to full speed (500kHz@8Mhz) ... both is per default zero
	//TWSR = 0;
	//TWBR = 0;
}

static void board_spi_init(void) {
	//SPI: enable, master, positive clock phase, msb first, SPI speed fosc/2
	SPCR = (0<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR0) | (0<<SPR1);
	SPSR = (1<<SPI2X);
}

static void board_usart_init(void) {
	// www.gjlay.de/helferlein/avr-uart-rechner.html
	UBRR0 = 0xc; // F_CPU / (BAUDRATE_CONS * 16L) - 1;
	UCSR0A |= (1<<U2X0);
	//Enable TXEN im Register UCR TX-Data Enable
	UCSR0B = (1 << TXEN0 | 1 << RXEN0 | 0<< RXCIE0);
}

void board_usart_write_char(const const char c) {
	//Warten solange bis vorherigs Zeichen gesendet wurde
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

unsigned char board_spi_put( const unsigned char value ) {
	SPDR = value;
	while( !(SPSR & (1<<SPIF)) ) ;
	return SPDR;
}

void board_turn_off() {
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	BOARD_LED_CHANNEL_BLUE_OFF;
	BOARD_LED_CHANNEL_RED_OFF;
	BOARD_LED_CHANNEL_GREEN_OFF;
	BOARD_LED_ERROR_OFF;
	xn297_goto_power_down();
	display_turn_off();
	//PCMSK2 |= 1<<PCINT21;
	//PCMSK0 |= (1<<PCINT6) | (1<<PCINT7);
	//PCICR |= (1<<PCIE2) | (1<<PCIE0);
	sleep_mode();
	// PCICR &= ~((1<<PCIE2) | (1<<PCIE0));
}

void board_test_loop() {
	
	BOARD_LED_CHANNEL_RED_ON;
	_delay_ms(500);
	BOARD_LED_CHANNEL_RED_OFF;
	BOARD_LED_CHANNEL_BLUE_ON;
	_delay_ms(500);
	BOARD_LED_CHANNEL_BLUE_OFF;
	BOARD_LED_CHANNEL_GREEN_ON;
	_delay_ms(500);
	BOARD_LED_CHANNEL_GREEN_OFF;
	BOARD_LED_ERROR_ON;
	_delay_ms(500);
	BOARD_LED_ERROR_OFF;
	
	display_update_status(0x01);
	// TODO: check flash ...
	
	// endlessloop for hardware testing:
	while (true) {
		if (BOARD_BUTTON_CHANNEL_DOWN) {
			BOARD_LED_CHANNEL_RED_ON;
		} else {
			BOARD_LED_CHANNEL_RED_OFF;
		}
		if (BOARD_BUTTON_RESET_DOWN) {
			BOARD_LED_CHANNEL_GREEN_ON;
		} else {
			BOARD_LED_CHANNEL_GREEN_OFF;
		}
		if (BOARD_BUTTON_START_DOWN) {
			BOARD_LED_CHANNEL_BLUE_ON;
		} else {
			BOARD_LED_CHANNEL_BLUE_OFF;
		}
		if (BOARD_BUTTON_START_DOWN && BOARD_BUTTON_RESET_DOWN && BOARD_BUTTON_CHANNEL_DOWN) {
			BOARD_LED_ERROR_ON;
		} else {
			BOARD_LED_ERROR_OFF;
		}
	}
		

}
