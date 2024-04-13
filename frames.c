typedef struct crsf_battery_sensor_s
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

void _write_battery_sensor_payload()
{
	sb_write_ui8(&_telemBuf, 11);	// Frame length
	sb_write_ui8(&_telemBuf, 0x08);	// Frame type
	sb_write_ui16(&_telemBuf, _telemetry.battery_sensor.voltage);
	sb_write_ui16(&_telemBuf, _telemetry.battery_sensor.current);
	sb_write_ui24(&_telemBuf, _telemetry.battery_sensor.capacity);
	sb_write_ui8(&_telemBuf, _telemetry.battery_sensor.percent);
}

void _write_link_statistics_payload()
{
	sb_write_ui8(&_telemBuf, 12);	// Frame length
	sb_write_ui8(&_telemBuf, 0x14);	// Frame type
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.uplink_rssi_ant_1);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.uplink_rssi_ant_2);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.uplink_package_success_rate);
	sb_write_i8(&_telemBuf, _telemetry.link_statistics.uplink_snr);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.diversity_active_antenna);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.rf_mode);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.uplink_tx_power);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.downlink_rssi);
	sb_write_ui8(&_telemBuf, _telemetry.link_statistics.downlink_package_success_rate);
	sb_write_i8(&_telemBuf, _telemetry.link_statistics.downlink_snr);
}


