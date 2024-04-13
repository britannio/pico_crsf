#pragma once
#include <stdint.h>
#include "pico/stdlib.h"

typedef struct link_statistics_s
{
    uint8_t rssi;
    uint8_t link_quality;
    int8_t snr;
    uint16_t tx_power;
} link_statistics_t;

void crsf_set_link_quality_threshold(uint8_t threshold);
void crsf_set_rssi_threshold(uint8_t threshold);
void crsf_set_on_rc_channels(void (*callback)(const uint16_t channels[16]));
void crsf_set_on_link_statistics(void (*callback)(const link_statistics_t *link_stats));
void crsf_set_on_failsafe(void (*callback)(const bool failsafe));
void crsf_begin(uart_inst_t *uart, uint8_t rx, uint8_t tx);
void crsf_end();
void crsf_process_frames();