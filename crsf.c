/**
 * @file crsf.c
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

#include "crsf.h"
#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <stdlib.h>
#include <string.h>

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

uart_inst_t *_uart = NULL;
uint8_t _incoming_frame[CRSF_MAX_FRAME_SIZE];
uint16_t _rc_channels[CRSF_MAX_CHANNELS];
link_statistics_t _link_statistics;
bool _failsafe = true;
uint8_t _link_quality_threshold = 70;
uint8_t _rssi_threshold = 105;

void (*rc_channels_callback)(const uint16_t channels[]);
void (*link_statistics_callback)(const link_statistics_t link_stats);
void (*failsafe_callback)(const bool failsafe);

uint8_t _telem_buf_data[CRSF_MAX_FRAME_SIZE];
buffer_t _telem_buf = {
    .buffer = _telem_buf_data,
    .capacity = CRSF_MAX_FRAME_SIZE,
    .offset = 0,
};
telemetry_t _telemetry;

enum
{
  CRSF_BATTERY_INDEX = 0,
  CRSF_CUSTOM_PAYLOAD_INDEX = 1,
  // Add new frame types above
  TELEMETRY_FRAME_TYPES
};

bool frameHasData[TELEMETRY_FRAME_TYPES] = {false};

/**
 * Sets the callback function to be called when RC channels are received.
 *
 * @param callback A function pointer to the callback function that takes an array of uint16_t channels as input.
 */
void crsf_set_on_rc_channels(void (*callback)(const uint16_t channels[16]))
{
  rc_channels_callback = callback;
}

/**
 * Sets the callback function for link statistics.
 *
 * This function sets the callback function that will be called when link statistics are available.
 *
 * @param callback A pointer to the callback function.
 */
void crsf_set_on_link_statistics(void (*callback)(const link_statistics_t link_stats))
{
  link_statistics_callback = callback;
}

/**
 * Sets the callback function to be called when a failsafe event occurs.
 *
 * @param callback A function pointer to the callback function that takes a boolean parameter indicating the failsafe status.
 */
void crsf_set_on_failsafe(void (*callback)(const bool failsafe))
{
  failsafe_callback = callback;
}

/**
 * Sets the link quality threshold for CRSF communication.
 *
 * The link quality threshold determines the minimum acceptable link quality for CRSF communication.
 * A lower threshold allows for more frames to be lost before the failsafe is triggered.
 *
 * @param threshold The link quality threshold value, ranging from 0 to 100.
 */
void crsf_set_link_quality_threshold(uint8_t threshold)
{
  _link_quality_threshold = threshold;
}

/**
 * Sets the RSSI (Received Signal Strength Indicator) threshold for CRSF communication.
 *
 * @param threshold The RSSI threshold value to set.
 */
void crsf_set_rssi_threshold(uint8_t threshold)
{
  _rssi_threshold = threshold;
}

/**
 * Initializes the CRSF communication by setting up the UART and configuring the RX and TX pins.
 *
 * @param uart The UART instance to be used for CRSF communication.
 * @param tx The TX pin number.
 * @param rx The RX pin number.
 */
void crsf_begin(uart_inst_t *uart, uint8_t tx, uint8_t rx)
{
  // TODO support PIO UART
  _uart = uart;
  // set up the UART
  uart_init(_uart, BAUD_RATE);
  gpio_set_function(tx, GPIO_FUNC_UART);
  gpio_set_function(rx, GPIO_FUNC_UART);
}

/**
 * @brief Ends the CRSF communication.
 *
 * This function deinitializes the UART used for CRSF communication.
 */
void crsf_end()
{
  uart_deinit(_uart);
}

void _process_rc_channels()
{
  const crsf_payload_rc_channels_packed_t *payload = (const crsf_payload_rc_channels_packed_t *)&_incoming_frame[3];
  _rc_channels[0] = payload->channel0;
  _rc_channels[1] = payload->channel1;
  _rc_channels[2] = payload->channel2;
  _rc_channels[3] = payload->channel3;
  _rc_channels[4] = payload->channel4;
  _rc_channels[5] = payload->channel5;
  _rc_channels[6] = payload->channel6;
  _rc_channels[7] = payload->channel7;
  _rc_channels[8] = payload->channel8;
  _rc_channels[9] = payload->channel9;
  _rc_channels[10] = payload->channel10;
  _rc_channels[11] = payload->channel11;
  _rc_channels[12] = payload->channel12;
  _rc_channels[13] = payload->channel13;
  _rc_channels[14] = payload->channel14;
  _rc_channels[15] = payload->channel15;
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

void _process_link_statistics()
{
  const crsf_payload_link_statistics_t *link_stats_payload = (const crsf_payload_link_statistics_t *)&_incoming_frame[3];
  _link_statistics.rssi = (link_stats_payload->diversity_active_antenna ? link_stats_payload->uplink_rssi_ant_2
                                                                        : link_stats_payload->uplink_rssi_ant_1);
  _link_statistics.link_quality = link_stats_payload->uplink_package_success_rate;
  _link_statistics.snr = link_stats_payload->uplink_snr;
  _link_statistics.tx_power = (link_stats_payload->uplink_tx_power < 9)
                                  ? tx_power_table[link_stats_payload->uplink_tx_power]
                                  : 0;
}

bool calculate_failsafe()
{
  return _link_statistics.link_quality <= _link_quality_threshold || _link_statistics.rssi >= _rssi_threshold;
}

uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len)
{
  static const uint8_t crsf_crc8tab[256] = {
      0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
      0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
      0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
      0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
      0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
      0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
      0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
      0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
      0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
      0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
      0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
      0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
      0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
      0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
      0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
      0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++)
  {
    crc = crsf_crc8tab[crc ^ *ptr++];
  }
  return crc;
}

void buf_reset(buffer_t *buf)
{
  if (buf)
  {
    buf->offset = 0;
  }
}

// Write an uint8_t to the buffer
void buf_write_ui8(buffer_t *buf, uint8_t data)
{
  if (buf && (buf->offset + sizeof(uint8_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = data;
    buf->offset += sizeof(uint8_t);
  }
}

// Write an int8_t to the buffer
void buf_write_i8(buffer_t *buf, int8_t data)
{
  if (buf && (buf->offset + sizeof(int8_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = data;
    buf->offset += sizeof(int8_t);
  }
}

// Write an uint16_t to the buffer
void buf_write_ui16(buffer_t *buf, uint16_t data)
{
  if (buf && (buf->offset + sizeof(uint16_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 1] = data & 0xFF;
    buf->offset += sizeof(uint16_t);
  }
}

// Write an int16_t to the buffer
void buf_write_i16(buffer_t *buf, int16_t data)
{
  if (buf && (buf->offset + sizeof(int16_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 1] = data & 0xFF;
    buf->offset += sizeof(int16_t);
  }
}

// Write an uint24_t to the buffer
void buf_write_ui24(buffer_t *buf, uint32_t data)
{
  if (buf && (buf->offset + 3 <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 16) & 0xFF;
    buf->buffer[buf->offset + 1] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 2] = data & 0xFF;
    buf->offset += 3;
  }
}

// Write an int24_t to the buffer
void buf_write_i24(buffer_t *buf, int32_t data)
{
  if (buf && (buf->offset + 3 <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 16) & 0xFF;
    buf->buffer[buf->offset + 1] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 2] = data & 0xFF;
    buf->offset += 3;
  }
}

// Write an uint32_t to the buffer
void buf_write_ui32(buffer_t *buf, uint32_t data)
{
  if (buf && (buf->offset + sizeof(uint32_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 24) & 0xFF;
    buf->buffer[buf->offset + 1] = (data >> 16) & 0xFF;
    buf->buffer[buf->offset + 2] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 3] = data & 0xFF;
    buf->offset += sizeof(uint32_t);
  }
}

// Write an int32_t to the buffer
void buf_write_i32(buffer_t *buf, int32_t data)
{
  if (buf && (buf->offset + sizeof(int32_t) <= buf->capacity))
  {
    buf->buffer[buf->offset] = (data >> 24) & 0xFF;
    buf->buffer[buf->offset + 1] = (data >> 16) & 0xFF;
    buf->buffer[buf->offset + 2] = (data >> 8) & 0xFF;
    buf->buffer[buf->offset + 3] = data & 0xFF;
    buf->offset += sizeof(int32_t);
  }
}

void _begin_frame()
{
  buf_reset(&_telem_buf);
  // Write sync byte
  buf_write_ui8(&_telem_buf, 0xC8);
}

void _end_frame()
{
  // Skip sync byte and frame length
  const uint8_t bytesToSkip = 2;
  const uint8_t *start = _telem_buf.buffer + bytesToSkip;
  const uint8_t length = _telem_buf.offset - bytesToSkip;
  const uint8_t crc = crsf_crc8(start, length);

  buf_write_ui8(&_telem_buf, crc);
}

// BEGIN gen_frames.dart
void _write_battery_sensor_payload()
{
  buf_write_ui8(&_telem_buf, 10);                            // Frame length
  buf_write_ui8(&_telem_buf, CRSF_FRAMETYPE_BATTERY_SENSOR); // Frame type
  buf_write_ui16(&_telem_buf, _telemetry.battery_sensor.voltage);
  buf_write_ui16(&_telem_buf, _telemetry.battery_sensor.current);
  buf_write_ui24(&_telem_buf, _telemetry.battery_sensor.capacity);
  buf_write_ui8(&_telem_buf, _telemetry.battery_sensor.percent);
}
// END gen_frames.dart

bool crsf_telem_update()
{
  bool updated = false;
  static int currentFrameType = 0;

  for (int i = 0; i < TELEMETRY_FRAME_TYPES; i++)
  {
    int frameTypeIndex = (currentFrameType + i) % TELEMETRY_FRAME_TYPES;

    if (frameHasData[frameTypeIndex])
    {
      _begin_frame();
      switch (frameTypeIndex)
      {
      case CRSF_BATTERY_INDEX:
        _write_battery_sensor_payload();
        break;
      case CRSF_CUSTOM_PAYLOAD_INDEX:
        buf_write_ui8(&_telem_buf, _telemetry.custom.length + 2);  // Frame length
        buf_write_ui8(&_telem_buf, CRSF_FRAMETYPE_CUSTOM_PAYLOAD); // Frame type
        for (size_t i = 0; i < _telemetry.custom.length; i++)
        {
          buf_write_ui8(&_telem_buf, _telemetry.custom.buffer[i]);
        }
        break;
      }
      _end_frame();
      updated = true;
      currentFrameType = (currentFrameType + 1) % TELEMETRY_FRAME_TYPES;
      break;
    }
  }

  return updated;
}

bool crsf_process_frame(uint8_t *frameIndex, uint8_t *frameLength, uint8_t *crcIndex, uint8_t currentByte)
{
  // Frame format:
  // [sync] [len] [type] [payload] [crc8]
  if (*frameIndex == 0)
  {
    // Should be the sync byte (0xC8)
    // "OpenTX/EdgeTX sends the channels packet starting with 0xEE instead of
    // 0xC8, this has been incorrect since the first CRSF implementation."
    if (currentByte != 0xC8 && currentByte != 0xEE)
    {
      DEBUG_WARN("Invalid sync byte: %04x", currentByte);
      return false;
    }
    _incoming_frame[*frameIndex++] = currentByte;
    return true;
  }
  else if (*frameIndex == 1)
  {
    // Should be the length byte
    _incoming_frame[*frameIndex++] = currentByte;
    *frameLength = currentByte;
    *crcIndex = *frameLength + 1;
    if (*frameLength < 2 || *frameLength > 62)
    {
      // Invalid frame length
      *frameIndex = 0;
      DEBUG_WARN("Frame length out of range: %d", *frameLength);
      return false;
    }
    return true;
  }
  else if (*frameIndex == *crcIndex)
  {
    // We have read the entire frame
    // Check the CRC
    if (crsf_crc8(_incoming_frame + 2, *frameLength - 1) == currentByte)
    {
      // Process the frame
      const uint8_t frameType = _incoming_frame[2];
      switch (frameType)
      {
      case CRSF_FRAMETYPE_LINK_STATISTICS:
        _process_link_statistics();
        if (link_statistics_callback != NULL)
        {
          link_statistics_callback(_link_statistics);
        }
        bool new_failsafe = calculate_failsafe();
        if (new_failsafe != _failsafe)
        {
          _failsafe = new_failsafe;
          if (failsafe_callback != NULL)
          {
            failsafe_callback(_failsafe);
          }
        }
        break;
      case CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
        _process_rc_channels();
        if (rc_channels_callback != NULL)
        {
          rc_channels_callback(_rc_channels);
        }
        break;
      default:
        DEBUG_WARN("Unknown frame type: %02x", frameType);
        break;
      }
      return true;
    }
    else
    {
      DEBUG_WARN("CRC check failed.");
    }
    // Reset the frame index
    *frameIndex = 0;
    return false;
  }
  else
  {
    _incoming_frame[*frameIndex++] = currentByte;
    return true;
  }

  return false;
}

void crsf_send_telem()
{
  // Send telemetry
  if (crsf_telem_update())
  {
    DEBUG_INFO("Sending telemetry frame");
    for (size_t i = 0; i < _telem_buf.offset; i++)
    {
      uart_putc(_uart, _telem_buf.buffer[i]);
    }
  }
}

/**
 * @brief Processes incoming CRSF frames.
 *
 * This function will attempt to process an incoming CRSF frame.
 * Once the UART queue is empty, a single pending telemetry frame will be sent.
 *
 * @attention Invoke this as frequently as possible to avoid missing frames.
 *
 * @related crsf_set_on_rc_channels
 * @related crsf_set_on_link_statistics
 * @related crsf_set_on_failsafe
 */
void crsf_process_frames() 
{
  // check if there is data available to read
  uint8_t frameIndex = 0;
  uint8_t frameLength = 0;
  uint8_t crcIndex = 0;
  // It takes 23.8095238095 µs to receive the next byte at 420000 baud
  while (uart_is_readable_within_us(_uart, 24))
  {
    // read the data
    uint8_t currentByte = uart_getc(_uart);
    crsf_process_frame(&frameIndex, &frameLength, &crcIndex, currentByte);
  }

  crsf_send_telem();
}

/**
 * Sets the battery data in the telemetry structure.
 *
 * @param voltage The battery voltage in dv
 * @param current The battery current in dA
 * @param capacity The battery capacity in mAH
 * @param percent The battery percentage remaining.
 */
void crsf_telem_set_battery_data(uint16_t voltage, uint16_t current, uint32_t capacity, uint8_t percent)
{
  _telemetry.battery_sensor.voltage = voltage;
  _telemetry.battery_sensor.current = current;
  _telemetry.battery_sensor.capacity = capacity;
  _telemetry.battery_sensor.percent = percent;
  frameHasData[CRSF_BATTERY_INDEX] = true;
}

void crsf_telem_set_custom_payload(uint8_t *data, uint8_t length)
{
  if (length > 60)
  {
    return;
  }
  memcpy(_telemetry.custom.buffer, data, length);
  _telemetry.custom.length = length;
  frameHasData[CRSF_CUSTOM_PAYLOAD_INDEX] = true;
}
