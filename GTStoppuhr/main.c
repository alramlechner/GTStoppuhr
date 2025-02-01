/*
 * GTStoppuhr.c
 *
 * Created: 08.01.2025 22:29:18
 * Author : alram
 */ 

#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdint.h>
#include <avr/sleep.h>
#include "Board.h"
#include "xn297l.h"
#include "eeprom_m24c32.h"
#include "stopwatch.h"
#include "driver/oled_ssd1306.h"
#include "GTProtocol.h"
#include "PushButtons.h"

static volatile bool xnReceivedData = false;
static unsigned char lastProcessedEvent[6];


static void sender_loop() {
	while(true) {
		BOARD_LED_CHANNEL_BLUE_ON;
		
		// TODO: send a packet ...
		
		BOARD_LED_CHANNEL_BLUE_OFF;
		_delay_ms(5000);
	}
}


int main(void)
{
	cli();
	board_init();
	
	console_write("\033[2J\033[H"); // clear screen + move curser to upper/left corner
	console_write("\n\rHello from GT Stoppuhr!");

	console_write("\n\rinit display ...");
	display_init();
	display_clearScreen();

	if (BOARD_BUTTON_CHANNEL_DOWN) {
		// hardware check after soldering
		display_update_status(0x00);
		board_test_loop();
	}

	if (BOARD_BUTTON_RESET_DOWN) {
		display_update_status(0x9);
		sender_loop();
	}
	
	/*
	eeprom_write_byte(0x0005, 0xFF);
	unsigned char val = eeprom_read_byte(0x0005);
	console_write("\n\reeprom address 5: %i", val);
	*/
	
	// 10ms from POR before accessing the xn297l
	BOARD_SPI_XN297_CE_INACTIVE;
	_delay_ms(10);
	xn297_reset();
	unsigned char xnStatus = xn297_get_status();
	// console_write("\n\rxn297l status register: %i", xnStatus);
	if (!(xnStatus&0b00001110)) {
		//TODO: we want to see the content of xnStatus on ... should be 00001110 after POR
		console_write("\n\rERR: No xn297L found");
		BOARD_LED_ERROR_ON;
	}

	// setup radio settings for GT
	gt_basic_radio_init();
	BOARD_XN297_IRQ_ENABLE;
	gt_goto_receive_mode();
	
	
	// bring xn297l to stb1 mode (CRC enabled is default on - don't turn it off)
	// xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_PWR_UP) | (1<<XN297L_REG_CONFIG_EN_CRC) |  (1<<XN297L_REG_CONFIG_EN_PM) | (1<<XN297L_REG_CONFIG_CRC_SCHEME));
	//_delay_ms(10); // needed by xn297l

	// bring xn297l to stb3 mode
	// xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_EN_PM));
	// _delay_us(50); // needed by xn297l
	
	// buttonStateBeforeIsr = 0xff;
	sei();

	// goto rx mode - start receiving: 
	// xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_PRIM_RX));
	stopwatch_init();
	display_update_clock(stopwatch_get_ms());
	set_sleep_mode(SLEEP_MODE_IDLE); // save 1-2mA in our case
	
    while (1) {
		push_buttons_mainloop_handler();		
		
		if (xnReceivedData == true) {
			unsigned char payload[6];
			xn297_cmd_ce_off();
			xn297_read_payload(payload, 6);
			//console_write("\n\rXN297L: Data received:");
			//for(int i=0; i<6; i++) {
				//console_write("0x%02X ", payload[i]);
			//}
			xnReceivedData = false;
			BOARD_XN297_IRQ_ENABLE;
			xn297_cmd_ce_on();
			
			// TODO: last process event should be stored for each device:
			if (lastProcessedEvent[3] == payload[3] && lastProcessedEvent[4] == payload[4] && lastProcessedEvent[5] == payload[5] ) {
				// ignore this event;
			} else {
				// new event
				stopwatch_reload_standbytimer();
				if (gt_payload_is_from_remote(payload) || gt_payload_is_from_starter(payload)) {
					stopwatch_start();
					BOARD_LED_CHANNEL_RED_ON;
				} else if (gt_payload_is_from_trigger(payload)) {
					stopwatch_stop();
					BOARD_LED_CHANNEL_RED_OFF;
				}
				for(int i=0; i<6; i++) {
					lastProcessedEvent[i] = payload[i];
				}
			}
		}
		if (stopwatch_consume_update_display() /*&& stopwatch_is_enabled()*/) {
			stopwatch_reload_standbytimer();
			display_update_clock(stopwatch_get_ms());
		}
		if (stopwatch_standby_timer_finished()) {
			board_turn_off();
			xn297_reset();
			gt_basic_radio_init();
			gt_goto_receive_mode();
			// stopwatch_init();
			display_init();
			display_clearScreen();
			display_update_clock(stopwatch_get_ms());
			set_sleep_mode(SLEEP_MODE_IDLE); // save 1-2mA in our case
			stopwatch_reload_standbytimer();
		}
		// needs timer for display update first.
		sleep_mode();
    }
}


// INT0 is IRQ line from xn297l
ISR(INT0_vect) { 
	//board_usart_write_char('!');
	BOARD_XN297_IRQ_DISABLE; // main need to re-enable it ...
	xnReceivedData = true;
}
