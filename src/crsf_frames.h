#include <stdint.h>

// BEGIN gen_frames.dart
typedef struct crsf_battery_sensor_s
{
	// voltage in dV (Big Endian)
	uint16_t voltage;
	// current in dA (Big Endian)
	uint16_t current;
	// used capacity in mAh (uint24_t)
	uint32_t capacity;
	// estimated battery remaining in percent (%)
	uint8_t percent;
} crsf_payload_battery_sensor_t;

typedef struct crsf_link_statistics_s
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

typedef struct telemetry_s
{
	crsf_payload_battery_sensor_t battery_sensor;
	crsf_payload_link_statistics_t link_statistics;
} telemetry_t;
// END gen_frames.dart