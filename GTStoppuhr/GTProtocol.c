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
// 4-6: random number (same for repeated events)

// value first byte in payload + channel num added
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
// static uint8_t currentChannel = GT_CHANNEL1_RED;

// variables need for transmit:
static uint8_t packetCountToSend = 0;
static const char* address[6];
static uint8_t channel[6];
static uint8_t payload[6][6];

// TODO: remember per device
static unsigned char lastProcessedEvent[6];

static uint8_t gt_map_color_to_channel(uint8_t color) {
	if (color == GT_COLOR_RED) {
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

static void gt_send_next_packet() {
	xn297_cmd_ce_off();
	uint8_t index = packetCountToSend % 3;
	xn297_write_register_1byte(XN297L_REG_RF_CH, channel[6-index]);
	xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, address[6-index], 5);
	xn297_cmd_ce_on();
	
	xn297_write_payload(payload[6-index], 6);
	
	_delay_ms(4); // hacky - could be done via timer
}

void gt_send_trigger_packet() {
	
	// connect wenn auf ROT gesendet werden soll: jeweils ca. 5ms Abstand
	//1: GT_CHANNEL1_RED		GT_ADDRESS_RED		0x13	GT_PREFIX_CONNECT
	//2: GT_CHANNEL2_RED		GT_ADDRESS_RED		0x14	GT_PREFIX_CONNECT
	//3: GT_CHANNEL1_GREEN		GT_ADDRESS_GREEN	0x14	GT_PREFIX_CONNECT
	//4: GT_CHANNEL2_GREEN		GT_ADDRESS_GREEN	0x15	GT_PREFIX_CONNECT
	//5: GT_CHANNEL1_BLUE		GT_ADDRESS_BLUE		0x15	GT_PREFIX_CONNECT
	//6: GT_CHANNEL2_BLUE		GT_ADDRESS_BLUE		0x13	GT_PREFIX_CONNECT
	//-w?
	//7: GT_CHANNEL1_RED		GT_ADDRESS_RED		0x13	GT_PREFIX_CONNECT
	//8: GT_CHANNEL2_RED		GT_ADDRESS_RED		0x14	GT_PREFIX_CONNECT
	//9: GT_CHANNEL1_GREEN		GT_ADDRESS_GREEN	0x14	GT_PREFIX_CONNECT
	//0: GT_CHANNEL2_GREEN		GT_ADDRESS_GREEN	0x15	GT_PREFIX_CONNECT
	//1: GT_CHANNEL1_BLUE		GT_ADDRESS_BLUE		0x15	GT_PREFIX_CONNECT
	//2: GT_CHANNEL2_BLUE		GT_ADDRESS_BLUE		0x13	GT_PREFIX_CONNECT
	//-w?
	//3: GT_CHANNEL1_RED		GT_ADDRESS_RED		0x13	GT_PREFIX_CONNECT
	// sample: 0x13 0x06 0xC8 0x8D 0x00 0x6E


	channel[0] = GT_CHANNEL1_RED;
	channel[1] = GT_CHANNEL2_RED;
	channel[2] = GT_CHANNEL1_GREEN;
	channel[3] = GT_CHANNEL2_GREEN;
	channel[4] = GT_CHANNEL1_BLUE;
	channel[5] = GT_CHANNEL2_BLUE;

	address[0] = GT_ADDRESS_RED;
	address[1] = GT_ADDRESS_RED;
	address[2] = GT_ADDRESS_GREEN;
	address[3] = GT_ADDRESS_GREEN;
	address[4] = GT_ADDRESS_BLUE;
	address[5] = GT_ADDRESS_BLUE;
	
	payload[0][0] = 0x13;
	payload[0][1] = 0x06;
	payload[0][2] = 0xC8;
	//payload[0][1] = 0x05;
	//payload[0][2] = 0x00;
	payload[0][3] = 0x8D;
	payload[0][4] = 0x00;
	payload[0][5] = 0x6E;

	payload[1][0] = 0x14;
	payload[1][1] = 0x06;
	payload[1][2] = 0xC8;
	payload[1][3] = 0x8D;
	payload[1][4] = 0x00;
	payload[1][5] = 0x6E;
	
	payload[2][0] = 0x14;
	payload[2][1] = 0x06;
	payload[2][2] = 0xC8;
	payload[2][3] = 0x8D;
	payload[2][4] = 0x00;
	payload[2][5] = 0x6E;

	payload[3][0] = 0x15;
	payload[3][1] = 0x06;
	payload[3][2] = 0xC8;
	payload[3][3] = 0x8D;
	payload[3][4] = 0x00;
	payload[3][5] = 0x6E;

	payload[4][0] = 0x15;
	payload[4][1] = 0x06;
	payload[4][2] = 0xC8;
	payload[4][3] = 0x8D;
	payload[4][4] = 0x00;
	payload[4][5] = 0x6E;
	
	payload[5][0] = 0x13;
	payload[5][1] = 0x06;
	payload[5][2] = 0xC8;
	payload[5][3] = 0x8D;
	payload[5][4] = 0x00;
	payload[5][5] = 0x6E;

	// uint8_t channel;
	// payload[0] = GT_BYTE0_OFFSET + currentColor;
	
	// emulate the connect:
	//for(uint8_t i=0; i<2; i++) {
		//payload[i+1] = pgm_read_byte(GT_PREFIX_CONNECT+i);
	//}
	
	//srand((lastProcessedEvent[3]<<8) | (lastProcessedEvent[4]<<8) );
	//payload[3] = 0x8d;// rand();
	//payload[4] = 0x00;//rand();
	//payload[5] = 0x6e;//rand();
	
	/*
	switch (currentColor) {
		case GT_COLOR_RED: 
			channel = GT_CHANNEL1_RED;
			break;
		case GT_COLOR_GREEN: 
			channel = GT_CHANNEL1_GREEN;
			break;
		case GT_COLOR_BLUE: 
			channel = GT_CHANNEL1_BLUE;
			break;
		default: return;
	}
	*/

	xn297_cmd_ce_off();
	// xn297_write_register_1byte(XN297L_REG_RF_CH, GT_CHANNEL2_BLUE);

	// enable transmitting mode (STB2)
	xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(0<<XN297L_REG_CONFIG_PRIM_RX));
	xn297_cmd_ce_on();
	
	// TODO: send multiple times on diff channels ...
	// start transmit by sending data:
	// xn297_write_payload(payload, 6);
	packetCountToSend = 18;
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
		// gt_basic_radio_init();
		// gt_goto_receive_mode();
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_GREEN, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_GREEN, 5);
	} else if (currentColor == GT_COLOR_BLUE) {
		BOARD_LED_CHANNEL_BLUE_ON;
		xn297_write_register_bytes_p(XN297L_REG_RX_ADDR_P0, GT_ADDRESS_BLUE, 5);
		xn297_write_register_bytes_p(XN297L_REG_TX_ADDR, GT_ADDRESS_BLUE, 5);
	}
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_map_color_to_channel(currentColor));
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
	xn297_write_register_1byte(XN297L_REG_RF_CH, gt_map_color_to_channel(currentColor));

	// enable receiver	
	xn297_write_register_1byte(XN297L_REG_CONFIG, (1<<XN297L_REG_CONFIG_EN_PM)|(1<<XN297L_REG_CONFIG_EN_CRC)|(1<<XN297L_REG_CONFIG_CRC_SCHEME)|(1<<XN297L_REG_CONFIG_PWR_UP)|(1<<XN297L_REG_CONFIG_PRIM_RX));
	xn297_cmd_ce_on();
}

void gt_received_data_ready() {
	// called in main once IRQ from xn297l fired:
	unsigned char xnStatus = xn297_get_status();

	// TX finished:
	if ((xnStatus&0x20) == 0x20) {
		console_write("TX!");
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
				
		// TODO: last process event should be stored for each device:
		if (lastProcessedEvent[3] == payload[3] && lastProcessedEvent[4] == payload[4] && lastProcessedEvent[5] == payload[5] ) {
			// ignore this event;
			} else {
			// new event
			stopwatch_reload_standbytimer();
			if (gt_payload_is_from_remote(payload) || gt_payload_is_from_starter(payload)) {
				stopwatch_start();
				} else if (gt_payload_is_from_trigger(payload)) {
				stopwatch_stop();
				} else {
				console_write("\n\rXN297L: Unknown data received:");
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