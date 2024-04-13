#include "crsf.h"
#include "crsf_crc.h"
#include "crsf_telemetry.h"
#include <hardware/uart.h>
#include <hardware/gpio.h>

#define BAUD_RATE 420000
#define CRSF_MAX_CHANNELS 16

uart_inst_t *_uart;
uint8_t _incoming_frame[64];
uint16_t _rc_channels[CRSF_MAX_CHANNELS];
link_statistics_t _link_statistics;
bool failsafe = false;

uint8_t link_quality_threshold = 70;
uint8_t rssi_threshold = 105;

void (*rc_channels_callback)(const uint16_t channels[]);

void (*link_statistics_callback)(const link_statistics_t *link_stats);

void (*failsafe_callback)(const bool failsafe);

void crsf_set_on_rc_channels(void (*callback)(const uint16_t channels[16])) {
  rc_channels_callback = callback;
}

void crsf_set_on_link_statistics(void (*callback)(const link_statistics_t *link_stats)) {
  link_statistics_callback = callback;
}

void crsf_set_on_failsafe(void (*callback)(const bool failsafe)) {
  failsafe_callback = callback;
}

void crsf_set_link_quality_threshold(uint8_t threshold) {
  link_quality_threshold = threshold;
}

void crsf_set_rssi_threshold(uint8_t threshold) {
  rssi_threshold = threshold;
}

void crsf_begin(uart_inst_t *uart, uint8_t rx, uint8_t tx) {
  // TODO support PIO UART
  _uart = uart;
  // set up the UART
  uart_init(_uart, BAUD_RATE);
  gpio_set_function(rx, GPIO_FUNC_UART);
  gpio_set_function(tx, GPIO_FUNC_UART);
}

void _process_rc_channels() {
  _rc_channels[0] = ((_incoming_frame[3] | _incoming_frame[4] << 8) & 0x07FF);
  _rc_channels[1] = ((_incoming_frame[4] >> 3 | _incoming_frame[5] << 5) & 0x07FF);
  _rc_channels[2] = ((_incoming_frame[5] >> 6 | _incoming_frame[6] << 2 | _incoming_frame[7] << 10) & 0x07FF);
  _rc_channels[3] = ((_incoming_frame[7] >> 1 | _incoming_frame[8] << 7) & 0x07FF);
  _rc_channels[4] = ((_incoming_frame[8] >> 4 | _incoming_frame[9] << 4) & 0x07FF);
  _rc_channels[5] = ((_incoming_frame[9] >> 7 | _incoming_frame[10] << 1 | _incoming_frame[11] << 9) & 0x07FF);
  _rc_channels[6] = ((_incoming_frame[11] >> 2 | _incoming_frame[12] << 6) & 0x07FF);
  _rc_channels[7] = ((_incoming_frame[12] >> 5 | _incoming_frame[13] << 3) & 0x07FF);
  _rc_channels[8] = ((_incoming_frame[14] | _incoming_frame[15] << 8) & 0x07FF);
  _rc_channels[9] = ((_incoming_frame[15] >> 3 | _incoming_frame[16] << 5) & 0x07FF);
  _rc_channels[10] = ((_incoming_frame[16] >> 6 | _incoming_frame[17] << 2 | _incoming_frame[18] << 10) & 0x07FF);
  _rc_channels[11] = ((_incoming_frame[18] >> 1 | _incoming_frame[19] << 7) & 0x07FF);
  _rc_channels[12] = ((_incoming_frame[19] >> 4 | _incoming_frame[20] << 4) & 0x07FF);
  _rc_channels[13] = ((_incoming_frame[20] >> 7 | _incoming_frame[21] << 1 | _incoming_frame[22] << 9) & 0x07FF);
  _rc_channels[14] = ((_incoming_frame[22] >> 2 | _incoming_frame[23] << 6) & 0x07FF);
  _rc_channels[15] = ((_incoming_frame[23] >> 5 | _incoming_frame[24] << 3) & 0x07FF);
}

const uint16_t tx_power_table[9] = {
        0,    // 0 mW
        10,   // 10 mW
        25,   // 25 mW
        100,  // 100 mW
        500,  // 500 mW
        1000, // 1 W
        2000, // 2 W
        250,  // 250 mW
        50    // 50 mW
};

void _process_link_statistics() {
  const crsf_payload_link_statistics_t *link_stats_payload = (const crsf_payload_link_statistics_t *) &_incoming_frame[3];
  _link_statistics.rssi = (link_stats_payload->diversity_active_antenna ? link_stats_payload->uplink_rssi_ant_2
                                                                        : link_stats_payload->uplink_rssi_ant_1);
  _link_statistics.link_quality = link_stats_payload->uplink_package_success_rate;
  _link_statistics.snr = link_stats_payload->uplink_snr;
  _link_statistics.tx_power = (link_stats_payload->uplink_tx_power < 9)
                              ? tx_power_table[link_stats_payload->uplink_tx_power] : 0;
}

bool calculate_failsafe() {
  return _link_statistics.link_quality <= link_quality_threshold || _link_statistics.rssi >= rssi_threshold;
}

void crsf_process_frames() {
  // check if there is data available to read
  uint8_t frameIndex = 0;
  uint8_t frameLength = 0;
  uint8_t crcIndex = 0;
  while (uart_is_readable(_uart)) {
    // read the data
    uint8_t currentByte = uart_getc(_uart);
    if (frameIndex == 0) {
      // Should be the sync byte (0xC8)
      if (currentByte != 0xC8) {
        continue;
      }
      _incoming_frame[frameIndex++] = currentByte;
    } else if (frameIndex == 1) {
      // Should be the length byte
      _incoming_frame[frameIndex++] = currentByte;
      frameLength = currentByte;
      crcIndex = frameLength + 1;
      if (frameLength < 2 || frameLength > 62) {
        // Invalid frame length
        frameIndex = 0;
        continue;
      }
    } else if (frameIndex == crcIndex) {
      // We have read the entire frame
      // Check the CRC
      if (crsf_crc8(_incoming_frame + 2, frameLength - 1) == currentByte) {
        // Process the frame
        const uint8_t frameType = _incoming_frame[2];
        switch (frameType) {
          case 0x14:
            _process_link_statistics();
            if (link_statistics_callback != NULL) {
              link_statistics_callback(&_link_statistics);
            }
            bool new_failsafe = calculate_failsafe();
            if (new_failsafe != failsafe) {
              failsafe = new_failsafe;
              if (failsafe_callback != NULL) {
                failsafe_callback(failsafe);
              }
            }
            break;
          case 0x16:
            _process_rc_channels();
            if (rc_channels_callback != NULL) {
              rc_channels_callback(_rc_channels);
            }
            break;
          default:
            break;
        }
      }
      // Reset the frame index
      frameIndex = 0;
    } else {
      _incoming_frame[frameIndex++] = currentByte;
    }
  }

  // Send telemetry
  if (crsf_telem_update()) {
    stream_buffer_t *telemBuf = crsf_telem_get_buffer();
    if (telemBuf) {
      for (size_t i = 0; i < telemBuf->offset; i++) {
        uart_putc(_uart, telemBuf->buffer[i]);
      }
    }
  }
}

void crsf_end() {
  uart_deinit(_uart);
}
