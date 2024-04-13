#include "crsf_telemetry.h"
#include "crsf_crc.h"


uint8_t _telemBufData[64];
stream_buffer_t _telemBuf = {
        .buffer = _telemBufData,
        .capacity = 64,
        .offset = 0,
};
telemetry_t _telemetry;

enum {
  CRSF_BATTERY_INDEX = 0,
  // Add new frame types above
  TELEMETRY_FRAME_TYPES
};

bool frameHasData[TELEMETRY_FRAME_TYPES] = {false};

void _begin_frame() {
  sb_reset(&_telemBuf);
  // Write sync byte
  sb_write_ui8(&_telemBuf, 0xC8);
}

void _end_frame() {
  // Skip sync byte and frame length
  const uint8_t bytesToSkip = 2;
  const uint8_t *start = _telemBuf.buffer + bytesToSkip;
  const uint8_t length = _telemBuf.offset - bytesToSkip;
  const uint8_t crc = crsf_crc8(start, length);

  sb_write_ui8(&_telemBuf, crc);
}

// BEGIN gen_frames.dart
void _write_battery_sensor_payload() {
  sb_write_ui8(&_telemBuf, 11);   // Frame length
  sb_write_ui8(&_telemBuf, 0x08); // Frame type
  sb_write_ui16(&_telemBuf, _telemetry.battery_sensor.voltage);
  sb_write_ui16(&_telemBuf, _telemetry.battery_sensor.current);
  sb_write_ui24(&_telemBuf, _telemetry.battery_sensor.capacity);
  sb_write_ui8(&_telemBuf, _telemetry.battery_sensor.percent);
}
// END gen_frames.dart

bool crsf_telem_update() {
  bool updated = false;
  static int currentFrameType = 0;

  for (int i = 0; i < TELEMETRY_FRAME_TYPES; i++) {
    int frameTypeIndex = (currentFrameType + i) % TELEMETRY_FRAME_TYPES;

    if (frameHasData[frameTypeIndex]) {
      _begin_frame();
      switch (frameTypeIndex) {
        case CRSF_BATTERY_INDEX:
          _write_battery_sensor_payload();
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

stream_buffer_t *crsf_telem_get_buffer() {
  if (_telemBuf.offset == 0) {
    return NULL;
  }
  return &_telemBuf;
}

void crsf_telem_set_battery_data(uint16_t voltage, uint16_t current, uint32_t capacity, uint8_t percent) {
  _telemetry.battery_sensor.voltage = voltage;
  _telemetry.battery_sensor.current = current;
  _telemetry.battery_sensor.capacity = capacity;
  _telemetry.battery_sensor.percent = percent;
  frameHasData[CRSF_BATTERY_INDEX] = true;
}

