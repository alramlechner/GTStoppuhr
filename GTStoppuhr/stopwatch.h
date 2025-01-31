/*
 * stopwatch.h
 *
 * Created: 19.01.2025 13:47:59
 *  Author: alram
 */ 


#ifndef STOPWATCH_H_
#define STOPWATCH_H_

#include <stdint.h>
#include <stdbool.h>

/** setup hardware timer */
void stopwatch_init();

/** start the stopwatch */
void stopwatch_start();

/** stop the stopwatch */
void stopwatch_stop();

/** reset the stopwatch */
void stopwatch_reset();

/** return the current value in ms */
uint_fast32_t stopwatch_get_ms();

/** should we update the display? */
bool stopwatch_consume_update_display();

bool stopwatch_is_enabled();

void stopwatch_start_debouncetimer(uint8_t num);
bool stopwatch_debouncetimer_finished(uint8_t num);
void stopwatch_reload_standbytimer();
bool stopwatch_standby_timer_finished();

#endif /* STOPWATCH_H_ */