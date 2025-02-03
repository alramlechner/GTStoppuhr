/*
 * GTProtocol.c
 *
 * Created: 25.01.2025 12:39:04
 *  Author: alram
 */ 

#include <stdbool.h>
#include <stdint.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "GTProtocol.h"
#include "xn297l.h"
#include "Board.h"
#include "stopwatch.h"

// Payload of received packet - 6 bytes long:
// 1: depends on channel: 0x13 (red), 0x14 (green), 0x15 (blue)
// 2: sending device (see GT_PREFIX defines byte 1)
// 3: status see: https://github.com/Ravensburger-Verlag-GmbH/GraviTrax-Connect/blob/main/GraviTrax-Connect-Python-Library/gravitraxconnect/gravitrax_constants.py#L32
// 4-5: random number (same for repeated events)
// 6: crc checksum

// value first byte in payload + color num added
#define GT_BYTE0_OFFSET 0x13

// Constants for byte 2 in payload (Sending device)
#define GT_PREFIX_TRIGGER   (0x01) //untested
#define GT_PREFIX_FINISH    (0x02)
#define GT_PREFIX_STARTER   (0x04)
#define GT_PREFIX_REMOTE    (0x05)
#define GT_PREFIX_BRDIGE    (0x06) // BRIDGE == CONNECT

// Constants for byte 3 in payload (Status/Receiver for message)
#define GT_STATUS_ALL			(0x00)
#define GT_STATUS_STARTER		(0x01)
#define GT_STATUS_SWITCH		(0x02)
#define GT_STATUS_BRIDGE		(0x03)
#define GT_STATUS_SOUND			(0x04)
#define GT_STATUS_LEVER			(0x06)
#define GT_STATUS_UNLOCK		(0xC8) // dec: 200
#define GT_STATUS_LOCK			(0xC9) // dec: 201
#define GT_STATUS_STARTER_PRESS (0xCA) // dec: 202

// Address used for xn297l to send/receive packets
const PROGMEM char GT_ADDRESS_RED[]   =  {0x67, 0x3a, 0xC2, 0x94, 0x7d};
const PROGMEM char GT_ADDRESS_GREEN[] =  {0x68, 0x3b, 0xC3, 0x95, 0x7e};
const PROGMEM char GT_ADDRESS_BLUE[]  =  {0x69, 0x3c, 0xC4, 0x96, 0x7f};

// RF channels used for xn297l to send/receive packets
const PROGMEM char GT_CHANNELS_RED[]   = {0x02, 0x45};
const PROGMEM char GT_CHANNELS_GREEN[] = {0x03, 0x46};
const PROGMEM char GT_CHANNELS_BLUE[]  = {0x04, 0x47};

// color current currently set to this device:
static uint8_t currentColor = GT_COLOR_RED;

static bool starterIsLocked = false;
// variables need for transmitting:
#define REPEAT_PACKET     6
static bool randSeedDone = false;

// store last message id to avoid double reaction on the same received packet
static unsigned char lastProcessedEvent[6];

static uint8_t gt_get_channel_for_current_color(const uint8_t alt) {
	if (currentColor == GT_COLOR_RED) {
		return pgm_read_byte(GT_CHANNELS_RED + alt%2);
	} else if (currentColor == GT_COLOR_GREEN) {
		return pgm_read_byte(GT_CHANNELS_GREEN + alt%2);
	} else { // if (currentColor == GT_COLOR_BLUE)
		return pgm_read_byte(GT_CHANNELS_BLUE + alt%2);
	}
}

/** ensure srand is called; to allow kind of randomness, don't call it in init() - we rely on random timer value */
static void gt_ensure_rand_seeded() {
	if (randSeedDone) 
		return;
	uint16_t currentTimerValue = TCNT1L;
	currentTimerValue += (TCNT1H<<8);
	srand(currentTimerValue);
	randSeedDone = true;
}

bool gt_payload_is_from_remote(const unsigned char *payload) {
	return (GT_PREFIX_REMOTE == payload[1]);
}

bool gt_payload_is_from_finish(const unsigned char *payload) {
	return (GT_PREFIX_FINISH == payload[1]);
}

bool gt_payload_is_from_starter(const unsigned char *payload) {
	return (GT_PREFIX_STARTER == payload[1]);
}

bool gt_payload_is_from_connect(const unsigned char *payload) {
	return (GT_PREFIX_BRDIGE == payload[1]);
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

bool gt_is_starter_locked() {
	return starterIsLocked;
}

static uint8_t gt_calc_checksum(const uint8_t payload[]) {
	uint16_t crc = 0;
	for(uint8_t i=0; i<5; i++) {
		crc += payload[i];
	}
	return crc % 256;
}

static void gt_send_packets(const uint8_t statusByte) {
	static uint8_t tx_payload[6];
	gt_ensure_rand_seeded();
	
	// switch to TX:
	xn297_cmd_ce_off();
	// enable transmitting mode (STB2)
	xn297_write_register_1byte(XN297L_REG_CONFIG, 
		(1<<XN297L_REG_CONFIG_EN_PM) | (1<<XN297L_REG_CONFIG_EN_CRC) | (1<<XN297L_REG_CONFIG_CRC_SCHEME) | (1<<XN297L_REG_CONFIG_PWR_UP) | (0<<XN297L_REG_CONFIG_PRIM_RX) | (1<<XN297L_REG_CONFIG_MASK_TX_DS));
	xn297_cmd_ce_on();
	
	// 1st packet - find random msg id and create CRC
	tx_payload[0] = GT_BYTE0_OFFSET + currentColor;
	tx_payload[1] = GT_PREFIX_BRDIGE;
	tx_payload[2] = statusByte;
	tx_payload[3] = rand()%256;
	tx_payload[4] = rand()%256;
	tx_payload[5] = gt_calc_checksum(tx_payload);
	//console_write("\n\rXN297L: TX Dump:");
	//for(int i=0; i<6; i++) {
		//console_write("0x%02X ", payload[i]);
	//}

	for(uint8_t i=0; i<REPEAT_PACKET; i++) {
		// switch between alternative channels:
		xn297_write_register_1byte(XN297L_REG_RF_CH, gt_get_channel_for_current_color(i));
		xn297_write_payload(tx_payload, 6);
		_delay_ms(4);
	}
	gt_goto_receive_mode();
}

void gt_send_trigger_packet() {
	gt_send_packets(GT_STATUS_ALL);
}

void gt_lock_starter() {
	gt_send_packets(GT_STATUS_LOCK);
	starterIsLocked = true;
}

void gt_unlock_starter() {
	gt_send_packets(GT_STATUS_UNLOCK);
	starterIsLocked = false;
}

void gt_set_current_color(const uint8_t newColor) {
	bool wasOn = xn297_is_ce_on();
	if (wasOn)
		xn297_cmd_ce_off();
	currentColor = newColor;
	BOARD_LED_CHANNEL_BLUE_OFF;
	BOARD_LED_CHANNEL_RED_OFF;
	BOARD_LED_CHANNEL_GREEN_OFF;
	switch(currentColor) {
		case GT_COLOR_RED:
			BOARD_LED_CHANNEL_RED_ON;
			xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_RED, 5);
			xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_RED, 5);
			break;
		case GT_COLOR_GREEN:
			BOARD_LED_CHANNEL_GREEN_ON;
			xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_GREEN, 5);
			xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_GREEN, 5);
			break;
		case GT_COLOR_BLUE:
			BOARD_LED_CHANNEL_BLUE_ON;
			xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_BLUE, 5);
			xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_BLUE, 5);
			break;
	}
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_get_channel_for_current_color(0));
	if (wasOn)
		xn297_cmd_ce_on();
}


void gt_basic_radio_init() {
	BOARD_SPI_XN297_CE_INACTIVE;
	xn297_reset();
	unsigned char xnStatus = xn297_get_status();
	if (!(xnStatus&0b00001110)) {
		console_write("\n\rERR: No xn297L found");
		BOARD_LED_ERROR_ON;
	}
		
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
		
	// change CE to SPI controlled - not needed as we have dedicated pin
	// xn297_write_register_1byte(XN297L_REG_FEATURE, 0x20);
	gt_set_current_color(currentColor);
}

void gt_goto_receive_mode() {
	xn297_cmd_ce_off();
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_get_channel_for_current_color(0));
	xn297_send_command(XN297L_CMD_FLUSH_TX);
	xn297_send_command(XN297L_CMD_FLUSH_RX);

	// enable receiver	
	xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_PRIM_RX)|(1<<XN297L_REG_CONFIG_MASK_TX_DS));
	xn297_cmd_ce_on();
}

void gt_mainloop_worker() {
	
	if (BOARD_XN297_IRQ_ACTIVE) {
		unsigned char xnStatus = xn297_get_status();
		//// TX finished:
		//if ((xnStatus&0x20) == 0x20) {
			//xn297_write_register_1byte(XN297L_REG_STATUS, 0x70); // clear interrupts
			//packetCountToSend --;
			////console_write("\r\nTX > %d", packetCountToSend);
			//BOARD_XN297_IRQ_ENABLE;
		//}
	
		// RX payload read:
		if ((xnStatus&0x0e) != 0x0e) {
			unsigned char payload[6];
			xn297_cmd_ce_off();
			xn297_read_payload(payload, 6);
			xn297_cmd_ce_on();

			//console_write("\n\rXN297L: RX Dump:");
			//for(int i=0; i<6; i++) {
				//console_write("0x%02X ", payload[i]);
			//}

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
				if ((gt_payload_is_from_remote(payload) || gt_payload_is_from_starter(payload))) {
					stopwatch_reload_standbytimer();
					if (!gt_is_starter_locked())
						stopwatch_start();
				} else if (gt_payload_is_from_finish(payload)) {
					stopwatch_reload_standbytimer();
					stopwatch_stop();
					_delay_ms(300); // better: wait for radio silence
					gt_unlock_starter();
				} else if (gt_payload_is_from_connect(payload)) {
					// signal from connect: should we start/stop? don't know ...
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
			BOARD_XN297_IRQ_ENABLE;
		}
	}
}

// INT0 is IRQ line from xn297l
// we need to int to wake-up from sleep
ISR(INT0_vect) {
	//board_usart_write_char('!');
	BOARD_XN297_IRQ_DISABLE;
}

