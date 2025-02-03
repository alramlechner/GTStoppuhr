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

	/*
	eeprom_write_byte(0x0005, 0xFF);
	unsigned char val = eeprom_read_byte(0x0005);
	console_write("\n\reeprom address 5: %i", val);
	*/
	
	// setup radio settings for GT
	gt_basic_radio_init();
	BOARD_XN297_IRQ_ENABLE;
	gt_goto_receive_mode();

	sei();

	stopwatch_init();
	display_update_clock(stopwatch_get_ms());
	set_sleep_mode(SLEEP_MODE_IDLE); // save 1-2mA in our case
	
    while (1) {
		push_buttons_mainloop_handler();		
		gt_mainloop_worker();
		if (stopwatch_consume_update_display()) {
			if (stopwatch_is_enabled())
				stopwatch_reload_standbytimer();
			display_update_clock(stopwatch_get_ms());
		}
		if (stopwatch_standby_timer_finished()) {
			board_turn_off();
			gt_basic_radio_init();
			gt_goto_receive_mode();
			display_init();
			display_clearScreen();
			display_update_clock(stopwatch_get_ms());
			set_sleep_mode(SLEEP_MODE_IDLE); // save 1-2mA in our case
			stopwatch_reload_standbytimer();
		}
		sleep_mode();
    }
}

