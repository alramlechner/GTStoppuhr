/*
 * oled_ssd1306.h
 *
 * Created: 19.01.2025 21:57:27
 *  Author: alram
 */ 


#ifndef OLED_SSD1306_H_
#define OLED_SSD1306_H_

#include <stdint.h>

/** init the display & turn it on */
void display_init();

/** clear screen: no pixel enabled */
void display_clearScreen();

/** stopwatch millisec */
void display_update_clock(uint32_t millisec);

/** show status instead of clock */
void display_update_status(uint8_t status) ;

/** set display contrast (brightness). Valid value 0-255 */
void display_set_contrast(uint8_t newContrast);

void display_turn_off();

#endif /* OLED_SSD1306_H_ */