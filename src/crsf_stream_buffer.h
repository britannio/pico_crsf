#pragma once
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint8_t *buffer;
  size_t capacity;
  size_t offset;
} stream_buffer_t;

stream_buffer_t* sb_init(size_t capacity);
void sb_reset(stream_buffer_t *sbuf);
void sb_free(stream_buffer_t *sbuf);

void sb_write_ui8(stream_buffer_t *sbuf, uint8_t data);
void sb_write_i8(stream_buffer_t *sbuf, int8_t data);
void sb_write_ui16(stream_buffer_t *sbuf, uint16_t data);
void sb_write_i16(stream_buffer_t *sbuf, int16_t data);
void sb_write_ui24(stream_buffer_t *sbuf, uint32_t data);
void sb_write_i24(stream_buffer_t *sbuf, int32_t data);
void sb_write_ui32(stream_buffer_t *sbuf, uint32_t data);
void sb_write_i32(stream_buffer_t *sbuf, int32_t data);

