#include <stdint.h>
#include <stdbool.h>
#include "crsf_stream_buffer.h"
#include "crsf_frames.h"

void crsf_telem_set_battery_data(uint16_t voltage, uint16_t current, uint32_t capacity, uint8_t percent);

bool crsf_telem_update();
stream_buffer_t* crsf_telem_get_buffer();