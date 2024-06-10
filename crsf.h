/**
 * @file crsf.h
 * @author Britannio Jarrett
 * @brief
 * @version 0.1
 * @date 2024-04-13
 *
 * @copyright Copyright (c) Britannio Jarrett 2024
 *
 * @section LICENSE
 * Licensed under the MIT License.
 * See https://github.com/britannio/pico_crsf/blob/main/LICENSE for more information.
 *
 */
#pragma once
#include <stdint.h>
#include "pico/stdlib.h"

#define BAUD_RATE 420000
#define CRSF_MAX_CHANNELS 16
#define CRSF_MAX_FRAME_SIZE 64
#define CRSF_DEBUG 0
#if CRSF_DEBUG
#include <stdio.h>
#define DEBUG_WARN(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG_INFO(...) printf(__VA_ARGS__)
#else
#define DEBUG_WARN(...)
#define DEBUG_INFO(...)
#endif

typedef struct
{
	uint8_t buffer[60];
	uint8_t length;
} crsf_payload_custom_t;

// BEGIN gen_frames.dart
// The values are CRSF channel values (0-1984). CRSF 172 represents 988us, CRSF 992 represents 1500us, and CRSF 1811 represents 2012us.
typedef struct __attribute__((packed))
{
	unsigned channel0 : 11;
	unsigned channel1 : 11;
	unsigned channel2 : 11;
	unsigned channel3 : 11;
	unsigned channel4 : 11;
	unsigned channel5 : 11;
	unsigned channel6 : 11;
	unsigned channel7 : 11;
	unsigned channel8 : 11;
	unsigned channel9 : 11;
	unsigned channel10 : 11;
	unsigned channel11 : 11;
	unsigned channel12 : 11;
	unsigned channel13 : 11;
	unsigned channel14 : 11;
	unsigned channel15 : 11;
} crsf_payload_rc_channels_packed_t;

typedef struct
{
	// voltage in dV (Big Endian)
	uint16_t voltage;
	// current in dA (Big Endian)
	uint16_t current;
	// used capacity in mAh
	uint32_t capacity;
	// estimated battery remaining in percent (%)
	uint8_t percent;
} crsf_payload_battery_sensor_t;

typedef struct
{
	// Uplink RSSI Ant. 1 ( dBm * -1 )
	uint8_t uplink_rssi_ant_1;
	// Uplink RSSI Ant. 2 ( dBm * -1 )
	uint8_t uplink_rssi_ant_2;
	// Uplink Package success rate / Link quality ( % )
	uint8_t uplink_package_success_rate;
	// Uplink SNR ( dB, or dB*4 for TBS I believe )
	int8_t uplink_snr;
	// Diversity active antenna ( enum ant. 1 = 0, ant. 2 = 1 )
	uint8_t diversity_active_antenna;
	// RF Mode ( 500Hz, 250Hz etc, varies based on ELRS Band or TBS )
	uint8_t rf_mode;
	// Uplink TX Power ( enum 0mW = 0, 10mW, 25 mW, 100 mW, 500 mW, 1000 mW, 2000mW, 50mW )
	uint8_t uplink_tx_power;
	// Downlink RSSI ( dBm * -1 )
	uint8_t downlink_rssi;
	// Downlink package success rate / Link quality ( % )
	uint8_t downlink_package_success_rate;
	// Downlink SNR ( dB )
	int8_t downlink_snr;
} crsf_payload_link_statistics_t;

typedef struct
{
	crsf_payload_rc_channels_packed_t rc_channels_packed;
	crsf_payload_battery_sensor_t battery_sensor;
	crsf_payload_link_statistics_t link_statistics;
	crsf_payload_custom_t custom;
} telemetry_t;

typedef enum
{
	CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16,
	CRSF_FRAMETYPE_BATTERY_SENSOR = 0x08,
	CRSF_FRAMETYPE_LINK_STATISTICS = 0x14,
	CRSF_FRAMETYPE_CUSTOM_PAYLOAD = 0x7F,

} frame_type_t;
// END gen_frames.dart

typedef struct
{
	uint8_t *buffer;
	size_t capacity;
	size_t offset;
} buffer_t;

typedef struct
{
	uint8_t rssi;
	uint8_t link_quality;
	int8_t snr;
	uint16_t tx_power;
} link_statistics_t;

enum
{
  CRSF_BATTERY_INDEX = 0,
  CRSF_CUSTOM_PAYLOAD_INDEX = 1,
  // Add new frame types above
  TELEMETRY_FRAME_TYPES
};

typedef struct
{
	uart_inst_t *uart = NULL;
	uint8_t incoming_frame[CRSF_MAX_FRAME_SIZE];
	uint16_t rc_channels[CRSF_MAX_CHANNELS];
	link_statistics_t link_statistics;
	bool failsafe;
	uint8_t link_quality_threshold;
	uint8_t rssi_threshold;

	void (*rc_channels_callback)(const uint16_t channels[]);
	void (*link_statistics_callback)(const link_statistics_t link_stats);
	void (*failsafe_callback)(const bool failsafe);

	uint8_t telem_buf_data[CRSF_MAX_FRAME_SIZE];
	buffer_t telem_buf;
	telemetry_t _telemetry;
	bool frameHasData[TELEMETRY_FRAME_TYPES];
} crsf_instance;

#define TICKS_TO_US(x) ((x - 992.0f) * 5.0f / 8.0f + 1500.0f)

#ifdef __cplusplus
extern "C"
{
#endif

	void crsf_init(crsf_instance* ins);
	void crsf_telem_set_battery_data(crsf_instance *ins, uint16_t voltage, uint16_t current, uint32_t capacity, uint8_t percent);
	void crsf_telem_set_custom_payload(crsf_instance *ins, uint8_t *data, uint8_t length);
	void crsf_set_link_quality_threshold(crsf_instance *ins, uint8_t threshold);
	void crsf_set_rssi_threshold(crsf_instance *ins, uint8_t threshold);
	void crsf_set_on_rc_channels(crsf_instance *ins, void (*callback)(const uint16_t channels[16]));
	void crsf_set_on_link_statistics(crsf_instance *ins, void (*callback)(const link_statistics_t link_stats));
	void crsf_set_on_failsafe(crsf_instance *ins, void (*callback)(const bool failsafe));
	void crsf_begin(crsf_instance *ins, uart_inst_t *uart, uint8_t rx, uint8_t tx);
	void crsf_end(crsf_instance *ins);
	void crsf_process_frames(crsf_instance *ins);
	void crsf_send_telem(crsf_instance *ins);
	bool crsf_process_frame(crsf_instance *ins, uint8_t *frameIndex, uint8_t *frameLength, uint8_t *crcIndex, uint8_t currentByte);

#ifdef __cplusplus
}
#endif