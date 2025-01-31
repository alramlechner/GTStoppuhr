/*
 * stopwatch.c
 *
 * Created: 19.01.2025 13:48:10
 *  Author: alram
 */ 
#include "stopwatch.h"
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Board.h"

static volatile uint_fast32_t stopWatchValueMs = 0;

// depends on timer settings:
#define TICKS_PER_MS 1000
#define MAX_SUPPORTED_MS 599000u
static volatile bool updateDisplay = false;
static volatile bool stopwatchStarted = false;
static volatile uint16_t standbyTimer = 0;
static volatile uint8_t debounceTimer[3]; // 3 buttons, 3 dedicated timer

static void reloadTimerMatchA() {
	uint16_t currentTimerValue = TCNT1L;
	currentTimerValue += (TCNT1H<<8);
	// trigger COMP A after 1msec
	currentTimerValue += TICKS_PER_MS;
	OCR1AH = (currentTimerValue>>8)&0xFF;
	OCR1AL = currentTimerValue&0xFF;
}

void stopwatch_init() {
	// Configure timer1 for 1M ticks/sec
	TCCR1B = (0<<CS12) | (1<<CS11) | (0<<CS10);
	stopwatch_reload_standbytimer();
	TIMSK1 |= (1<<TOIE1) | (1<<OCIE1A); // enable interrupts
}

void stopwatch_reload_standbytimer() {
	standbyTimer = 60000; // 1min to standby
}

bool stopwatch_standby_timer_finished() {
	return standbyTimer == 0;
}

void stopwatch_start() {
	// stop & clear if timer is running:
	// TIMSK1 &= ~(1<<OCIE1A); // disable interrupt

	stopWatchValueMs = 0;
	reloadTimerMatchA();
	stopwatchStarted = true;

	// To do a 16-bit write, the high byte must be written before the low byte. For a 16-bit read, the low byte must be read before the high byte.
	//TCNT0 = 255-125; // call ISR after 125 ticks
	//TCNT1H = 0x00; // check if timer change is okay with other timer usages...
	//TCNT1L = 0x00;
	
	// trigger COMP A after 1msec
	//OCR1AH = (TICKS_PER_MS>>8)&0xFF;
	//OCR1AL = TICKS_PER_MS&0xFF;
	
//	isrOverFlows = 1000; // at zero one second is over ...
	
	// TIMSK1 |= (1<<OCIE1A); // enable interrupt
}

void stopwatch_reset() {
	stopWatchValueMs = 0;
}

void stopwatch_stop() {
	// TIMSK1 &= ~(1<<OCIE1A); // disable COMP A interrupt
	stopwatchStarted = false;
}

uint_fast32_t stopwatch_get_ms() {
	return stopWatchValueMs;
}

bool stopwatch_consume_update_display() {
	if (updateDisplay) {
		updateDisplay = false;
		return true;
	}
	return false;
}

bool stopwatch_is_enabled() {
	return stopwatchStarted;
}

void stopwatch_start_debouncetimer(uint8_t num) {
	debounceTimer[num] = 30; // 60ms
}

bool stopwatch_debouncetimer_finished(uint8_t num) {
	return debounceTimer[num] == 0;
}

// StopWatch ms trigger
ISR(TIMER1_COMPA_vect) {
	reloadTimerMatchA();
	if (stopwatchStarted) {
		stopWatchValueMs ++;
		if (stopWatchValueMs > MAX_SUPPORTED_MS) {
			stopWatchValueMs = MAX_SUPPORTED_MS;
			stopwatch_stop();
		}
	}
	for(uint8_t i=0; i< sizeof(debounceTimer); i++) {
		if (debounceTimer[i] > 0) {
			debounceTimer[i]--;
		}
	}
	if (standbyTimer >= 0) {
		standbyTimer --;
	}
}

// awake from sleep, update display
ISR(TIMER1_OVF_vect) {
	// do nothing - just awake from sleep
	updateDisplay = true;
}
