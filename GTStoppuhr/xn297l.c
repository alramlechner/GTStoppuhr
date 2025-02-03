/*
 * xn297l.c
 *
 * Created: 11.01.2025 10:14:41
 *  Author: alram
 */ 

#include <stdint-gcc.h>
#include <avr/pgmspace.h>
#include "xn297l.h"
#include "Board.h"

// Register mask for READ/WRITE register command
#define REGISTER_MASK 0x1F
// value to be send after CMD_ACTIATION:
#define ACTIVATE_VALUE       0x73
#define DEACTIVATE_VALUE     0x8c

// check if CE is on (xn297 sending or receiving enabled)
bool xn297_is_ce_on() {
	return BOARD_SPI_XN297_CE_IS_ACTIVE;
}

void xn297_send_command(const uint_fast8_t cmd) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(cmd);
	BOARD_SPI_XN297_CS_DESELECT;
}

static void xn297_send_command_with_1byte(const uint_fast8_t cmd, const uint_fast8_t data) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(cmd);
	board_spi_put(data);
	BOARD_SPI_XN297_CS_DESELECT;
}


void xn297_write_register_1byte(const uint_fast8_t registerNum, const unsigned char value) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_WRITE_REGISTER | (registerNum&REGISTER_MASK));
	board_spi_put(value);
	BOARD_SPI_XN297_CS_DESELECT;
}

void xn297_write_register_bytes(const uint_fast8_t registerNum, const unsigned char *value, const uint8_t length) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_WRITE_REGISTER | (registerNum&REGISTER_MASK));
	for(int i=0; i<length; i++)
		board_spi_put(value[i]);
	BOARD_SPI_XN297_CS_DESELECT;
}

void xn297_write_register_bytes_p(const uint_fast8_t registerNum, const char *value_p, const uint8_t length) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_WRITE_REGISTER | (registerNum&REGISTER_MASK));
	for(int i=0; i<length; i++)
		board_spi_put(pgm_read_byte(value_p+i));
	BOARD_SPI_XN297_CS_DESELECT;
}

unsigned char xn297_get_status() {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_READ_REGISTER | XN297L_REG_STATUS);
	unsigned char ret = board_spi_put(0x00);
	BOARD_SPI_XN297_CS_DESELECT;
	return ret;
}

uint_fast8_t xn297_read_payload(unsigned char destination[], const uint8_t len) {
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_READ_RX_PAYLOAD);
	for(int i=0; i<len; i++)
		destination[i] = board_spi_put(0x00);
	BOARD_SPI_XN297_CS_DESELECT;
	xn297_send_command(XN297L_CMD_FLUSH_RX);
	xn297_write_register_1byte(XN297L_REG_STATUS, 0x70); // clear interrupts
	return len;
}

void xn297_write_payload(unsigned char data[], const uint8_t len) {
	// xn297_send_command(XN297L_CMD_FLUSH_TX);
	// xn297_write_register_1byte(XN297L_REG_STATUS, 0x70); // clear interrupts
	BOARD_SPI_XN297_CS_SELECT;
	board_spi_put(XN297L_CMD_WRITE_TX_PAYLOAD);
	for(int i=0; i<len; i++)
		board_spi_put(data[i]);
	BOARD_SPI_XN297_CS_DESELECT;
}

void xn297_reset() {
	xn297_send_command_with_1byte(XN297L_CMD_RST_FSPI, 0x5A);
	xn297_send_command_with_1byte(XN297L_CMD_RST_FSPI, 0xA5);	
	xn297_cmd_ce_off();
	xn297_send_command(XN297L_CMD_FLUSH_TX);
	xn297_send_command(XN297L_CMD_FLUSH_RX);
	xn297_write_register_1byte(XN297L_REG_STATUS, 0x70); // clear interrupts
}

void xn297_cmd_activate() {
	xn297_send_command_with_1byte(XN297L_CMD_ACTIVATION, 0x73);
}

void xn297_cmd_deactivate() {
	xn297_send_command_with_1byte(XN297L_CMD_ACTIVATION, 0x8C);
}

void xn297_cmd_ce_on() {
	//xn297_send_command_with_1byte(XN297L_CMD_CE_FSPI_ON, 0x00);
	BOARD_SPI_XN297_CE_ACTIVE;
}

void xn297_cmd_ce_off() {
	//xn297_send_command_with_1byte(XN297L_CMD_CE_FSPI_OFF, 0x00);
	BOARD_SPI_XN297_CE_INACTIVE;
}

void xn297_goto_power_down() {
	xn297_write_register_1byte(XN297L_REG_CONFIG, 0x00);
		// (0<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(0<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_PRIM_RX));

}

/*
static void xn297_read_register(const uint_fast8_t registerNum, const uint_fast8_t len, unsigned char values[]) {
	BOARD_SPI_XN297_CS_SELECT;
	statusRegister = board_spi_put(XN297L_CMD_READ_REGISTER | (registerNum&REGISTER_MASK));
	for(uint_fast8_t i=0; i<len; i++) {
		values[i] = board_spi_put(0x00);
	}
	BOARD_SPI_XN297_CS_DESELECT;
}

static void xn297_write_register(const uint_fast8_t registerNum, const uint_fast8_t len, const unsigned char values[]) {
	BOARD_SPI_XN297_CS_SELECT;
	statusRegister = board_spi_put(XN297L_CMD_WRITE_REGISTER | (registerNum&REGISTER_MASK));
	for(uint_fast8_t i=0; i<len; i++) {
		board_spi_put(values[i]);
	}
	BOARD_SPI_XN297_CS_DESELECT;
}

static void xn297_write_paylod(const char data[], const uint_fast8_t length ) {
	
}

static void xn297_read_paylod(const char data[], const uint_fast8_t length ) {
	
}
*/

