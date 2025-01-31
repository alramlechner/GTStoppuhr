/*
 * eeprom_m24c32.c
 *
 * Created: 19.01.2025 10:12:46
 *  Author: alram
 */ 

#include "eeprom_m24c32.h"
#include "i2cmaster.h"

#define BOARD_AD24C32_ADRESS 0xA0

unsigned char eeprom_read_byte(const uint_fast16_t address) {
	i2c_start_wait(BOARD_AD24C32_ADRESS+I2C_WRITE);     // set device address and write mode
	i2c_write((address>>8)&0x00FF);								// write address (MSB+LSB)
	i2c_write(address&0x00FF);
	i2c_rep_start(BOARD_AD24C32_ADRESS+I2C_READ);       // set device address and read mode
	unsigned char ret = i2c_readNak();                    // read one byte from EEPROM
	i2c_stop();
	return ret;
}

void eeprom_write_byte(const uint_fast16_t address, const unsigned char value) {
	i2c_start_wait(BOARD_AD24C32_ADRESS+I2C_WRITE);     // set device address and write mode
	i2c_write(address>>8);								// write address (MSB+LSB)
	i2c_write(address&0x00FF);
	i2c_write(value);                        // write value 0x75 to EEPROM
	i2c_stop();                             // set stop conditon = release bus
}

