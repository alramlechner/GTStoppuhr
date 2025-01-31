/*
 * GTProtocol.c
 *
 * Created: 25.01.2025 12:39:04
 *  Author: alram
 */ 

#include <stdbool.h>
#include <avr/pgmspace.h>
#include "GTProtocol.h"
#include "xn297l.h"
#include "Board.h"

const PROGMEM char GT_PREFIX_TRIGGER[] = {0x13, 0x02, 0x00};
const PROGMEM char GT_PREFIX_STARTER[] = {0x13, 0x04, 0xCA};
const PROGMEM char GT_PREFIX_REMOTE[] =  {0x13, 0x05, 0x00};

const PROGMEM char GT_ADDRESS_RED[]   =  {0x67, 0x3a, 0xC2, 0x94, 0x7d};
const PROGMEM char GT_ADDRESS_GREEN[] =  {0x68, 0x3b, 0xC3, 0x95, 0x7e};
const PROGMEM char GT_ADDRESS_BLUE[]  =  {0x69, 0x3c, 0xC4, 0x96, 0x7f};
	
static bool gt_prefix_matches(const char *cmd_p, const unsigned char *payload) {
	for(uint8_t i=0; i<3; i++) {
		if (pgm_read_byte(cmd_p+i) != payload[i]) {
			return false;
		}
	}
	return true;
}

bool gt_payload_is_from_remote(const unsigned char *payload) {
	return gt_prefix_matches(GT_PREFIX_REMOTE, payload);
}

bool gt_payload_is_from_trigger(const unsigned char *payload) {
	return gt_prefix_matches(GT_PREFIX_TRIGGER, payload);
}

bool gt_payload_is_from_starter(const unsigned char *payload) {
	return gt_prefix_matches(GT_PREFIX_STARTER, payload);
}

void gt_basic_radio_init() {
		BOARD_SPI_XN297_CE_INACTIVE;
		
		// enable RX pipe 0
		xn297_write_register_1byte(XN297L_REG_EN_RXADDR, 0x01);
		
		// RX/TX Address width to 5 bytes
		xn297_write_register_1byte(XN297L_REG_SETUP_AW, 0x03);
		
		// uint8_t channel = 0;
		// unsigned char address[6];

		// blue received from trigger:
		//address[0] = 0x69;
		//address[1] = 0x3c;
		//address[2] = 0xC4;
		//address[3] = 0x96;
		//address[4] = 0x7f;
		//channel = 0x04;
		// channel = 0x47; // unknown
		
		// green received from trigger:
		//address[0] = 0x68;
		//address[1] = 0x3b;
		//address[2] = 0xC3;
		//address[3] = 0x95;
		//address[4] = 0x7e;
		//channel = 0x03;
		//channel = 0x46; // unknown

		// red received from trigger:
		//address[0] = 0x67;
		//address[1] = 0x3a;
		//address[2] = 0xC2;
		//address[3] = 0x94;
		//address[4] = 0x7d;
		//// channel = 0x02;
		//channel = 0x45; // unknown
		
		// channel
		// xn297_write_register_1byte(XN297L_REG_RF_CH, channel);
		
		// packet length
		xn297_write_register_1byte(XN297L_REG_RX_PW_P0, 0x06);

		// disable dynamic payload length
		xn297_write_register_1byte(XN297L_REG_DYNPD, 0x00);
		
		// output power, rate 1mbps
		xn297_write_register_1byte(XN297L_REG_RF_SETUP, 0x19);
		
		// xn297_cmd_activate(); // not sure why needed ...
		
		// disable re-transmit
		xn297_write_register_1byte(XN297L_REG_SETUP_RETR, 0x00);
		
		// disable auto ack
		xn297_write_register_1byte(XN297L_REG_EN_AA, 0x00);
		
		// change CE to SPI controlled
		// xn297_write_register_1byte(XN297L_REG_FEATURE, 0x20);
		

}

void gt_goto_receive_mode() {
		// currently hard coded: red + 1st channel
		xn297_write_register_1byte(XN297L_REG_RF_CH, GT_CHANNEL1_RED);
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_RED, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_RED, 5);

		// enable receiver	
		xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_PRIM_RX));
		xn297_cmd_ce_on();
}