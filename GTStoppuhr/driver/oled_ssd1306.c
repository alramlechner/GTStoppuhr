/*
 * oled_ssd1306.c
 *
 * Created: 19.01.2025 21:57:39
 *  Author: alram
 */ 

#include "oled_ssd1306.h"

#include <stdint.h>
#include "Arial_40.h"
#include "i2cmaster.h"
#include "../Board.h"

#if OLED_WITH_SMALL_FONT == 1
#include "display/font6x8.h"
#endif

// #define SSD1306_I2C_ADDRESS 0x3c // 0x3c or 0x3d depends on pin setup (first charge from aliexpress is 0x3c)
//#define SSD1306_I2C_ADDRESS 0x78 // HS HS91L02W2C01 DataSheet
#define SSD1306_I2C_ADDRESS 0x78 

static int Chip_I2C_MasterSend(const uint8_t *buff, uint8_t len) {
	i2c_start_wait(SSD1306_I2C_ADDRESS + I2C_WRITE);     // set device address and write mode
	for(uint_fast8_t i=0; i<len; i++) {
		i2c_write(buff[i]);
	}
	i2c_stop();
	return 0;
}


void display_init() {
	uint8_t init[] = {
		0x00, // continues mode & control command
		//0xAE, // display off
		//0x00, // set lower column address
		//0x10, // set higher column address
		//0x00, // set display start line
		//0xB0, // set page address
		//0x81, // contract control
		//0xFF, // 128
		//0xA1, // set segment remap
		//0xA6, // normal/reverse
		//0xA8, // multiplex ratio
		//0x1F, // duty=1/32
		//0x8C, // com scan direction
		//0xD3, // set display offset
		//0x00,
		//0xD5, // set osc division
		//0x80,
		//0xD9, // set pre-charge period
		//0x1F,
		//0xDA, // set COM pins
		//0x00,
		//0xDB, // set vcomh
		//0x40,
		//0x8D, // set charge pump enable
		//0x14,
		//0xAF, // Display on
		
		
//		0xD3, // Display offset
//		0x00,
//		0x40, // Startline: 0x40-0x7f
		0xA1, // Segment re-map: column adress 127 => mapped to seg0 (A0 is default, A1 re-mapped)
//		0x81, // Display contrast
//		0xA0,
//		0xA4, // display on (0xa5 all on)
//		0xA6, // set display normal
		0x8D, // enable charge pump regulator
		0x14,
		0x20 | 0x00, // 0x00: horizontal adressing mode, 0x01: vertical, 0x02: page adressing mode
		0x00,
		0xC8, // COM Output/Scan direction 0xC0/0xC8
		0xAF  // turn on display
	};
	Chip_I2C_MasterSend(init, sizeof(init));
}

void display_turn_off() {
	uint8_t init[] = {
		0x00, // continues mode & control command
		0xAE // display off
		};
	Chip_I2C_MasterSend(init, sizeof(init));
}

static void display_init_cursor() {
	uint8_t init_current_address[] = {
		0x00, // continues mode & control command
		0x21, // set column address (start&current, end)
		0,
		127,
		0x22, // set page address (start&current, end)
		0,
		7
	};
	Chip_I2C_MasterSend(init_current_address, sizeof(init_current_address));
}

void display_clearScreen() {
	display_init_cursor();
	uint8_t segdata[129] = {0};
	segdata[0] = 0b01000000; // continues + data
	for (int page=0; page<8; page++) {
		Chip_I2C_MasterSend(segdata, sizeof(segdata));
	}
}

void display_set_contrast(uint8_t newContrast) {
	uint8_t contrast[] = {
		0x00, // continues mode & control command
		0x81, // contrast
		newContrast
	};
	Chip_I2C_MasterSend(contrast, sizeof(contrast));

}

// code from: https://github.com/jdmorise/AVR_SSD1306_bigchar_demo
#define DISPLAY_WIDTH	128
#define DISPLAY_HEIGHT	64

#define CHAR_HEIGHT_40  40
#define CHAR_WIDTH_40  30 // 1024 bytes

#define CHAR_HEIGHT CHAR_HEIGHT_40
#define CHAR_WIDTH CHAR_WIDTH_40

static void lcd_gotoxy(uint8_t x, uint8_t y){
    if( x > (DISPLAY_WIDTH) || y > (DISPLAY_HEIGHT/8-1)) return;// out of display
    //x = x * 8;					// one char: 6 pixel width
	uint8_t init_current_address[] = {
		0x00, // continues mode & control command
		0xb0+y,
		0x21, // set column address (start&current, end)
		x,
		0x7f
	};
	Chip_I2C_MasterSend(init_current_address, sizeof(init_current_address));
//    uint8_t commandSequence[] = {0xb0+y, 0x21, x, 0x7f};
//    lcd_command(commandSequence, sizeof(commandSequence));
}

static void lcd_put_bigc(const char* addr_p, uint8_t column, uint8_t char_width, uint8_t general_width, uint8_t char_height){
	uint8_t x_offset = ((general_width - char_width)>>1);
	for (uint8_t i = 0; i < (char_height >> 3); i++) {
		uint8_t dataToSend[35]; // 31 is well known max...
		int bytesToSend = 0;

		lcd_gotoxy(column, 1+i);
		dataToSend[bytesToSend] = 0x40;
		bytesToSend++;

		for (uint8_t j = 0; j < general_width; j++)
			if(j < x_offset) {
				// lcd_send_i2c_byte(0x00);
				dataToSend[bytesToSend] = 0x00;
				bytesToSend++;
				}
			else {
				if(j < (x_offset + char_width)) {
					dataToSend[bytesToSend] = pgm_read_byte(addr_p + (j-x_offset+char_width*i));
					bytesToSend++;
					}
				else {
					dataToSend[bytesToSend] = 0x00;
					bytesToSend++;
				}
			}
		Chip_I2C_MasterSend(dataToSend, bytesToSend);
    }
}

static void lcd_paint_point(uint8_t x, uint8_t y) {
	uint8_t dataToSend[20];
	uint8_t bytesToSend = 0;

	lcd_gotoxy(x, y);
 	for(uint8_t i=0; i<4; i++) {
		bytesToSend = 0;
		dataToSend[bytesToSend] = 0x40;
		bytesToSend++;
		dataToSend[bytesToSend] = 0b11110000;
		bytesToSend++;
		Chip_I2C_MasterSend(dataToSend, bytesToSend);
	}
}

void display_update_clock(uint32_t millisec) {
	
	uint8_t min, sec, hsec;
	
	min = millisec/(uint32_t)60000;
	if (min > 9) min = 9;
	millisec -= min * (uint32_t)60000;
	sec = millisec / (uint32_t)1000;
	millisec -= sec*(uint32_t)1000;
	hsec = millisec/100;

	lcd_put_bigc (char_addr_p[min], 0, char_width[min], CHAR_WIDTH, CHAR_HEIGHT);
	// doppelpunkt ...
	lcd_paint_point(CHAR_WIDTH, 2);
	lcd_paint_point(CHAR_WIDTH, 4);
	lcd_put_bigc (char_addr_p[sec/10], 34, char_width[sec/10], CHAR_WIDTH, CHAR_HEIGHT);
	lcd_put_bigc (char_addr_p[sec%10], 61, char_width[sec%10], CHAR_WIDTH, CHAR_HEIGHT);
	lcd_paint_point(61+CHAR_WIDTH, 4);
	lcd_put_bigc (char_addr_p[hsec], 96, char_width[hsec], CHAR_WIDTH, CHAR_HEIGHT);
	
	
	//digit_2 = a%10;
	//a = a/10;
	//digit_1 = a%10;
	//digit_4 = b%10;
	//b = b/10;
	//digit_3 = b%10;
	// lcd_update_4(min, digit_2, digit_3, digit_4);
}

void display_update_status(uint8_t status) {
	lcd_put_bigc (char_addr_p[status], 0, char_width[status], CHAR_WIDTH, CHAR_HEIGHT);
}
