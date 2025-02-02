/*
 * GTProtocol.c
 *
 * Created: 25.01.2025 12:39:04
 *  Author: alram
 */ 

#include <stdbool.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include "GTProtocol.h"
#include "xn297l.h"
#include "Board.h"
#include "stopwatch.h"

// Payload of received packet - 6 bytes long:
// 1: depends on channel: 0x13 (red), 0x14 (green), 0x15 (blue)
// 2-3: sending device (see GT_PREFIX defines)
//      2nd byte might be filter for specific receivers only (unclear)
// 4-5: random number (same for repeated events)
// 6: crc checksum

// value first byte in payload + color num added
#define GT_BYTE0_OFFSET 0x13

const PROGMEM char GT_PREFIX_TRIGGER[] =   {0x02, 0x00};
const PROGMEM char GT_PREFIX_STARTER[] =   {0x04, 0xCA};
const PROGMEM char GT_PREFIX_REMOTE[] =    {0x05, 0x00};
const PROGMEM char GT_PREFIX_CONNECT[] =   {0x06, 0xC8};

const PROGMEM char GT_ADDRESS_RED[]   =  {0x67, 0x3a, 0xC2, 0x94, 0x7d};
const PROGMEM char GT_ADDRESS_GREEN[] =  {0x68, 0x3b, 0xC3, 0x95, 0x7e};
const PROGMEM char GT_ADDRESS_BLUE[]  =  {0x69, 0x3c, 0xC4, 0x96, 0x7f};

#define GT_CHANNEL1_RED 0x02
#define GT_CHANNEL2_RED 0x45

#define GT_CHANNEL1_GREEN 0x03
#define GT_CHANNEL2_GREEN 0x46

#define GT_CHANNEL1_BLUE 0x04
#define GT_CHANNEL2_BLUE 0x47


static uint8_t currentColor = GT_COLOR_RED;

// variables need for transmitting:
static uint8_t packetCountToSend = 0;
static uint8_t payload[5];
static bool randSeedDone = false;

// store last message id to avoid double reaction on the same received packet
static unsigned char lastProcessedEvent[6];

static uint8_t gt_get_channel_for_current_color() {
	if (currentColor == GT_COLOR_RED) {
		return GT_CHANNEL1_RED;
	} else if (currentColor == GT_COLOR_GREEN) {
		return GT_CHANNEL1_GREEN;
	} else if (currentColor == GT_COLOR_BLUE) {
		return GT_CHANNEL1_BLUE;
	}
	return GT_CHANNEL1_RED;
}

static bool gt_prefix_matches(const char *cmd_p, const unsigned char *payload) {
	for(uint8_t i=0; i<2; i++) {
		if (pgm_read_byte(cmd_p+i) != payload[i+1]) {
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
	// return gt_prefix_matches(GT_PREFIX_TRIGGER, payload);
}

bool gt_payload_is_from_starter(const unsigned char *payload) {
	return gt_prefix_matches(GT_PREFIX_STARTER, payload);
	//return gt_prefix_matches(GT_PREFIX_STARTER, payload);
}

void gt_switch_to_next_color() {
	if (currentColor == GT_COLOR_RED) {
		gt_set_current_color(GT_COLOR_BLUE);
	} else if (currentColor == GT_COLOR_GREEN) {
		gt_set_current_color(GT_COLOR_RED);
	} else if (currentColor == GT_COLOR_BLUE) {
		gt_set_current_color(GT_COLOR_GREEN);
	}
}

static uint8_t gt_calc_checksum(const uint8_t payload[]) {
	uint16_t crc = 0;
	for(uint8_t i=0; i<5; i++) {
		crc += payload[i];
	}
	return crc % 256;
}

static void gt_send_next_packet() {
	// switch channel between 1st and 2nd (for the given color)
	//xn297_cmd_ce_off();
	//xn297_write_register_1byte(XN297L_REG_RF_CH, GT_CHANNEL1_GREEN);
	//xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, address[6-index], 5);
	//xn297_cmd_ce_on();
	payload[5] = gt_calc_checksum(payload);
	xn297_write_payload(payload, 6);
	_delay_ms(1); // hacky - could be done via timer
}

void gt_send_trigger_packet() {
	payload[0] = GT_BYTE0_OFFSET + currentColor;
	payload[1] = pgm_read_byte(GT_PREFIX_REMOTE);
	payload[2] = pgm_read_byte(GT_PREFIX_REMOTE+1);
	if (!randSeedDone) {
		// we assume some random time since timer start and trigger packet to be sent:
		uint16_t currentTimerValue = TCNT1L;
		currentTimerValue += (TCNT1H<<8);
		srand(currentTimerValue);
		randSeedDone = true;
	}
	payload[3] = rand()%256;
	payload[4] = rand()%256;

	xn297_cmd_ce_off();
	// enable transmitting mode (STB2)
	xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(0<<XN297L_REG_CONFIG_PRIM_RX));
	xn297_cmd_ce_on();
	
	// start transmit by sending data:
	packetCountToSend = 12;
	gt_send_next_packet();
}

void gt_set_current_color(const uint8_t newColor) {
	bool wasOn = xn297_is_ce_on();
	if (wasOn)
		xn297_cmd_ce_off();
	currentColor = newColor;
	BOARD_LED_CHANNEL_BLUE_OFF;
	BOARD_LED_CHANNEL_RED_OFF;
	BOARD_LED_CHANNEL_GREEN_OFF;
	if (currentColor == GT_COLOR_RED) {
		BOARD_LED_CHANNEL_RED_ON;
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_RED, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_RED, 5);
	} else if (currentColor == GT_COLOR_GREEN) {
		BOARD_LED_CHANNEL_GREEN_ON;
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_GREEN, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_GREEN, 5);
	} else if (currentColor == GT_COLOR_BLUE) {
		BOARD_LED_CHANNEL_BLUE_ON;
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_BLUE, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_BLUE, 5);
	}
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_get_channel_for_current_color());
	if (wasOn)
		xn297_cmd_ce_on();
}


void gt_basic_radio_init() {
	BOARD_SPI_XN297_CE_INACTIVE;
		
	// enable RX pipe 0
	xn297_write_register_1byte(XN297L_REG_EN_RXADDR, 0x01);
		
	// RX/TX Address width to 5 bytes
	xn297_write_register_1byte(XN297L_REG_SETUP_AW, 0x03);
		
	// packet length
	xn297_write_register_1byte(XN297L_REG_RX_PW_P0, 0x06);

	// disable dynamic payload length
	xn297_write_register_1byte(XN297L_REG_DYNPD, 0x00);
		
	// output power, rate 1mbps
	xn297_write_register_1byte(XN297L_REG_RF_SETUP, 0x19);
		
	xn297_cmd_activate(); // needed for TX
		
	// disable re-transmit
	xn297_write_register_1byte(XN297L_REG_SETUP_RETR, 0x00);
		
	// disable auto ack
	xn297_write_register_1byte(XN297L_REG_EN_AA, 0x00);
		
	// change CE to SPI controlled
	// xn297_write_register_1byte(XN297L_REG_FEATURE, 0x20);
	gt_set_current_color(currentColor);	
}

void gt_goto_receive_mode() {
	xn297_cmd_ce_off();
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_get_channel_for_current_color());

	// enable receiver	
	xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_PRIM_RX));
	xn297_cmd_ce_on();
}

void gt_received_data_ready() {
	// called in main once IRQ from xn297l fired:
	unsigned char xnStatus = xn297_get_status();

	// TX finished:
	if ((xnStatus&0x20) == 0x20) {
		// console_write("TX!");
		xn297_write_register_1byte(XN297L_REG_STATUS, 0x70); // clear interrupts
		packetCountToSend --;
		if (packetCountToSend > 0)
			gt_send_next_packet();
		else {
			gt_goto_receive_mode();
		}
	}
	
	// RX payload read:
	if ((xnStatus&0x0e) != 0x0e) {
		unsigned char payload[6];
		xn297_cmd_ce_off();
		xn297_read_payload(payload, 6);
		xn297_cmd_ce_on();

		console_write("\n\rXN297L: RX Dump:");
		for(int i=0; i<6; i++) {
			console_write("0x%02X ", payload[i]);
		}

		uint8_t crc = gt_calc_checksum(payload);
		if (payload[5] != crc) {
			console_write("\n\rXN297L: Invalid checksum:");
			for(int i=0; i<6; i++) {
				console_write("0x%02X ", payload[i]);
			}
		}
		
		// TODO: we should remember last 12 message id's (per device?)
		if (lastProcessedEvent[3] == payload[3] && lastProcessedEvent[4] == payload[4] && lastProcessedEvent[5] == payload[5] ) {
			// ignore this event;
		} else {
			// new event
			if (gt_payload_is_from_remote(payload) || gt_payload_is_from_starter(payload)) {
				stopwatch_reload_standbytimer();
				stopwatch_start();
			} else if (gt_payload_is_from_trigger(payload)) {
				stopwatch_reload_standbytimer();
				stopwatch_stop();
			} else {
				console_write("\n\rXN297L: Received packet from unknown sender:");
				for(int i=0; i<6; i++) {
					console_write("0x%02X ", payload[i]);
				}
			}
			for(int i=0; i<6; i++) {
				lastProcessedEvent[i] = payload[i];
			}
		}
	}

}