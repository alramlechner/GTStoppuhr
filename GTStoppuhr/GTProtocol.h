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

#define GT_COLOR_RED    0
#define GT_COLOR_GREEN  1
#define GT_COLOR_BLUE   2

void gt_basic_radio_init();
uint8_t gt_get_current_channel();
void gt_switch_to_next_color();
void gt_set_current_color(const uint8_t newChannel);
void gt_goto_receive_mode();

void gt_mainloop_worker();
void gt_send_trigger_packet();

bool gt_payload_is_from_remote(const unsigned char *payload);
bool gt_payload_is_from_finish(const unsigned char *payload);
bool gt_payload_is_from_starter(const unsigned char *payload);
void gt_lock_starter();
void gt_unlock_starter();
bool gt_is_starter_locked();


#endif /* GTPROTOCOL_H_ */