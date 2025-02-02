/*
 * xn297l.h
 *
 * Created: 11.01.2025 10:14:30
 *  Author: alram
 */ 


#ifndef XN297L_H_

#include <stdbool.h>
#include <avr/pgmspace.h>

#define XN297L_H_xn297_write_register_1byte

#define XN297L_REG_CONFIG      0x00
#define XN297L_REG_EN_AA       0x01 // enable auto acknowledgement
#define XN297L_REG_EN_RXADDR   0x02 // enable RX addresses
#define XN297L_REG_SETUP_AW    0x03 // Setup of Address Widths (common for all data pipes)
#define XN297L_REG_SETUP_RETR  0x04 // Setup of Automatic Retransmission
#define XN297L_REG_RF_CH       0x05 // RF Channel
#define XN297L_REG_RF_SETUP    0x06 // RF Setup Register
#define XN297L_REG_STATUS      0x07
#define XN297L_REG_OBSERVE_TX  0x08 // Transmit observe register
#define XN297L_REG_CD          0x09 // Received Power Detector ...
#define XN297L_REG_RX_ADDR_P0  0x0A // Receive address data pipe 0. 5 Bytes maximum length.
#define XN297L_REG_RX_ADDR_P1  0x0B // Receive address data pipe 1. 5 Bytes maximum length.
#define XN297L_REG_RX_ADDR_P2  0x0C // Receive address data pipe 2. Only LSB. MS Bytes are equal to RX_ADDR_P1[39:8].
#define XN297L_REG_RX_ADDR_P3  0x0D // Receive address data pipe 3. Only LSB. MS Bytes are equal to RX_ADDR_P1[39:8].
#define XN297L_REG_RX_ADDR_P4  0x0E // Receive address data pipe 4. Only LSB. MS Bytes are equal to RX_ADDR_P1[39:8].
#define XN297L_REG_RX_ADDR_P5  0x0F // Receive address data pipe 5. Only LSB. MS Bytes are equal to RX_ADDR_P1[39:8].
#define XN297L_REG_TX_ADDR     0x10 // Transmit address
#define XN297L_REG_RX_PW_P0    0x11 // Number of bytes in RX payload in data pipe 1 (0 not used, 1-64)
#define XN297L_REG_RX_PW_P1    0x12 // Number of bytes in RX payload in data pipe 2 (0 not used, 1-64)
#define XN297L_REG_RX_PW_P2    0x13 // Number of bytes in RX payload in data pipe 3 (0 not used, 1-64)
#define XN297L_REG_RX_PW_P3    0x14 // Number of bytes in RX payload in data pipe 4 (0 not used, 1-64)
#define XN297L_REG_RX_PW_P4    0x15 // Number of bytes in RX payload in data pipe 5 (0 not used, 1-64)
#define XN297L_REG_RX_PW_P5    0x16 // Number of bytes in RX payload in data pipe 6 (0 not used, 1-64)
#define XN297L_REG_FIFO_STATUS 0x17
#define XN297L_REG_DEMOD_CAL   0x19 // DEMOD_CAL
#define XN297L_REG_RF_CAL2     0x1A // RF_CAL2
#define XN297L_REG_DEM_CAL2    0x1B // DEM_CAL2
#define XN297L_REG_DYNPD       0x1C // Enable dynamic payload length
#define XN297L_REG_FEATURE     0x1D // Feature Register (IRQ pin, CE pin control)
#define XN297L_REG_RF_CAL      0x1E // RF_CAL
#define XN297L_REG_BB_CAL      0x1F // Special Function Register

// Bits in CONFIG register:
#define XN297L_REG_CONFIG_PRIM_RX      0
#define XN297L_REG_CONFIG_PWR_UP       1
#define XN297L_REG_CONFIG_CRC_SCHEME   2
#define XN297L_REG_CONFIG_EN_CRC       3
#define XN297L_REG_CONFIG_MASK_MAX_RT  4
#define XN297L_REG_CONFIG_MASK_TX_DS   5
#define XN297L_REG_CONFIG_MASK_RX_DR   6
#define XN297L_REG_CONFIG_EN_PM        7


void xn297_reset();
unsigned char xn297_get_status();
// bool xn297_has_data_in_rx_fifo();
// uint_fast8_t xn297_pipe_with_data_in_rc_fifo();

/** read dynamic payload and returns the length read from fifo */
uint_fast8_t xn297_read_payload(unsigned char destination[], const uint8_t len);

void xn297_write_payload(unsigned char data[], const uint8_t len);

void xn297_cmd_activate();
void xn297_cmd_deactivate();

void xn297_cmd_ce_on();
void xn297_cmd_ce_off();
bool xn297_is_ce_on();

void xn297_goto_power_down();

void xn297_write_register_1byte(const uint_fast8_t registerNum, const unsigned char value);
void xn297_write_register_bytes(const uint_fast8_t registerNum, const unsigned char *value, const uint8_t length);
void xn297_write_register_bytes_p(const uint_fast8_t registerNum, const char *value_p, const uint8_t length);


#endif /* XN297L_H_ */