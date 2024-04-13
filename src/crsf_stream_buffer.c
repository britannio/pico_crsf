#include "crsf_stream_buffer.h"

// Initialize a stream buffer
stream_buffer_t *sb_init(size_t capacity) {
  stream_buffer_t *sbuf = malloc(sizeof(stream_buffer_t));
  if (sbuf) {
    sbuf->buffer = malloc(capacity);
    if (sbuf->buffer) {
      sbuf->capacity = capacity;
      sbuf->offset = 0;
    } else {
      free(sbuf);
      sbuf = NULL;
    }
  }
  return sbuf;
}

void sb_reset(stream_buffer_t *sbuf) {
  if (sbuf) {
    sbuf->offset = 0;
  }
}

// Write an uint8_t to the buffer
void sb_write_ui8(stream_buffer_t *sbuf, uint8_t data) {
  if (sbuf && (sbuf->offset + sizeof(uint8_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = data;
    sbuf->offset += sizeof(uint8_t);
  }
}

// Write an int8_t to the buffer
void sb_write_i8(stream_buffer_t *sbuf, int8_t data) {
  if (sbuf && (sbuf->offset + sizeof(int8_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = data;
    sbuf->offset += sizeof(int8_t);
  }
}

// Write an uint16_t to the buffer
void sb_write_ui16(stream_buffer_t *sbuf, uint16_t data) {
  if (sbuf && (sbuf->offset + sizeof(uint16_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = data & 0xFF;
    sbuf->offset += sizeof(uint16_t);
  }
}

// Write an int16_t to the buffer
void sb_write_i16(stream_buffer_t *sbuf, int16_t data) {
  if (sbuf && (sbuf->offset + sizeof(int16_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = data & 0xFF;
    sbuf->offset += sizeof(int16_t);
  }
}

// Write an uint24_t to the buffer
void sb_write_ui24(stream_buffer_t *sbuf, uint32_t data) {
  if (sbuf && (sbuf->offset + 3 <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 16) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 2] = data & 0xFF;
    sbuf->offset += 3;
  }
}

// Write an int24_t to the buffer
void sb_write_i24(stream_buffer_t *sbuf, int32_t data) {
  if (sbuf && (sbuf->offset + 3 <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 16) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 2] = data & 0xFF;
    sbuf->offset += 3;
  }
}

// Write an uint32_t to the buffer
void sb_write_ui32(stream_buffer_t *sbuf, uint32_t data) {
  if (sbuf && (sbuf->offset + sizeof(uint32_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 24) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = (data >> 16) & 0xFF;
    sbuf->buffer[sbuf->offset + 2] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 3] = data & 0xFF;
    sbuf->offset += sizeof(uint32_t);
  }
}

// Write an int32_t to the buffer
void sb_write_i32(stream_buffer_t *sbuf, int32_t data) {
  if (sbuf && (sbuf->offset + sizeof(int32_t) <= sbuf->capacity)) {
    sbuf->buffer[sbuf->offset] = (data >> 24) & 0xFF;
    sbuf->buffer[sbuf->offset + 1] = (data >> 16) & 0xFF;
    sbuf->buffer[sbuf->offset + 2] = (data >> 8) & 0xFF;
    sbuf->buffer[sbuf->offset + 3] = data & 0xFF;
    sbuf->offset += sizeof(int32_t);
  }
}


// Free the buffer
void sb_free(stream_buffer_t *sbuf) {
  if (sbuf) {
    free(sbuf->buffer);
    free(sbuf);
  }
}
