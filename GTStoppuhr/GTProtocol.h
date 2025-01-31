/*
 * GTProtocol.h
 *
 * Created: 25.01.2025 12:13:54
 *  Author: alram
 */ 


#ifndef GTPROTOCOL_H_
#define GTPROTOCOL_H_

#include <stdbool.h>
#include <avr/pgmspace.h>

#define GT_CHANNEL1_RED 0x02
#define GT_CHANNEL2_RED 0x45

#define GT_CHANNEL1_GREEN 0x03
#define GT_CHANNEL2_GREEN 0x46

#define GT_CHANNEL1_BLUE 0x04
#define GT_CHANNEL2_BLUE 0x47

void gt_basic_radio_init();
void gt_goto_receive_mode();

bool gt_payload_is_from_remote(const unsigned char *payload);
bool gt_payload_is_from_trigger(const unsigned char *payload);
bool gt_payload_is_from_starter(const unsigned char *payload);

#endif /* GTPROTOCOL_H_ */