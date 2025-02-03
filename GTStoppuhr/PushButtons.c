/*
 * PushButtons.c
 *
 * Created: 01.02.2025 10:19:27
 *  Author: alram
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "PushButtons.h"
#include "stopwatch.h"
#include "Board.h"
#include "GTProtocol.h"

static uint8_t buttonStateBeforeIsr = 0x00;
static uint8_t logicalButtonState = 0x00;
static uint8_t notConsumedButtonState = 0x00;

// called from ISR; debounce buttons ...
static void update_button_state(bool currentButtonState, uint8_t buttonNumber) {
	if (currentButtonState != ((buttonStateBeforeIsr>>buttonNumber)&0x01) ) {
		if (currentButtonState)
			buttonStateBeforeIsr |= (1<<buttonNumber);
		else
			buttonStateBeforeIsr &= ~(1<<buttonNumber);
		
		if (stopwatch_debouncetimer_finished(buttonNumber)) {
			// accept button event:
			stopwatch_start_debouncetimer(buttonNumber);
			if (currentButtonState)
				// button pressed
				logicalButtonState |= (1<<buttonNumber);
			else
				// button release
				logicalButtonState &= ~(1<<buttonNumber);
			notConsumedButtonState |= (1<<buttonNumber);
		} else
			// restart the timer ...
			stopwatch_start_debouncetimer(buttonNumber);
	}
}

#define BUTTON_START_NUM    0
#define BUTTON_CHANNEL_NUM  1
#define BUTTON_RESET_NUM    2

// Button event handler:
static void button_start_pressed() {
	// console_write("\n\rStart pressed");
	if (stopwatch_is_enabled()) {
		gt_unlock_starter();
		stopwatch_stop();
	} else {
		gt_lock_starter();
		gt_send_trigger_packet();
		stopwatch_start();
	}
}

static void button_start_released() {
	// console_write("\n\rStart released");
}

static void button_channel_pressed() {
	// console_write("\n\rChannel pressed");
	stopwatch_stop();
	stopwatch_reset();
	gt_switch_to_next_color();
}

static void button_channel_released() {
	// console_write("\n\rChannel released");
}

static void button_reset_pressed() {
	// console_write("\n\rReset pressed");
	if (stopwatch_is_enabled())
		stopwatch_stop();
	stopwatch_reset();
}

static void button_reset_released() {
	// console_write("\n\rReset released");
}

void push_buttons_mainloop_handler() {
	// Handle button state changes:
	if ((notConsumedButtonState>>BUTTON_START_NUM)&0x01 && (logicalButtonState>>BUTTON_START_NUM)&0x01) {
		notConsumedButtonState &= ~(1<<BUTTON_START_NUM);
		button_start_pressed();
	} else if ((notConsumedButtonState>>BUTTON_START_NUM)&0x01 && !((logicalButtonState>>BUTTON_START_NUM)&0x01)) {
		notConsumedButtonState &= ~(1<<BUTTON_START_NUM);
		button_start_released();
	}

	if ((notConsumedButtonState>>BUTTON_CHANNEL_NUM)&0x01 && (logicalButtonState>>BUTTON_CHANNEL_NUM)&0x01) {
		notConsumedButtonState &= ~(1<<BUTTON_CHANNEL_NUM);
		button_channel_pressed();
	} else if ((notConsumedButtonState>>BUTTON_CHANNEL_NUM)&0x01 && !((logicalButtonState>>BUTTON_CHANNEL_NUM)&0x01)) {
		notConsumedButtonState &= ~(1<<BUTTON_CHANNEL_NUM);
		button_channel_released();
	}

	if ((notConsumedButtonState>>BUTTON_RESET_NUM)&0x01 && (logicalButtonState>>BUTTON_RESET_NUM)&0x01) {
		notConsumedButtonState &= ~(1<<BUTTON_RESET_NUM);
		button_reset_pressed();
	} else if ((notConsumedButtonState>>BUTTON_RESET_NUM)&0x01 && !((logicalButtonState>>BUTTON_RESET_NUM)&0x01)) {
		notConsumedButtonState &= ~(1<<BUTTON_RESET_NUM);
		button_reset_released();
	}
}

ISR(PCINT0_vect) {
	update_button_state(BOARD_BUTTON_CHANNEL_DOWN, BUTTON_CHANNEL_NUM);
	update_button_state(BOARD_BUTTON_RESET_DOWN, BUTTON_RESET_NUM);
}

ISR(PCINT2_vect) {
	update_button_state(BOARD_BUTTON_START_DOWN, BUTTON_START_NUM);
}

