/*
 * eeprom_m24c32.h
 *
 * Created: 19.01.2025 10:12:33
 *  Author: alram
 */ 
#include <stdint.h>

#ifndef EEPROM_M24C32_H_
#define EEPROM_M24C32_H_


unsigned char eeprom_read_byte(const uint_fast16_t address);
void eeprom_write_byte(const uint_fast16_t address, const unsigned char value);



#endif /* EEPROM_M24C32_H_ */