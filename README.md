# Pico CRSF

A CRSF library for the Raspberry Pi Pico.

## Usage

Connect a receiver to UART0/UART1 pins on your Raspberry Pi Pico.

Add `crsf.c` and `crsf.h` to your project.

Use as follows:

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "crsf.h"

void on_rc_channels(const uint16_t channels[16]) {}
void on_link_stats(const link_statistics_t link_stats) {}
void on_failsafe(const bool failsafe) {}

int main() {
    stdio_init_all();
    crsf_set_link_quality_threshold(70);
    crsf_set_rssi_threshold(105);

    crsf_set_on_rc_channels(on_rc_channels);
    crsf_set_on_link_statistics(on_link_stats);
    crsf_set_on_failsafe(on_failsafe);

    crsf_begin(uart0, 1, 0);
    for (;;) crsf_process_frames();
}
```

Use `crsf_end(<uart>)` once done.



## CRSF Message Format
https://github.com/crsf-wg/crsf/wiki/Message-Format

Message format:
`[sync] [len] [type] [payload] [crc8]`

Frames are defined via the `Payload` enum in `gen_frames.dart`.

Running `dart gen_frames.dart` from a terminal (see https://dart.dev/get-dart)
will print the generated C structs and code to convert these structs into
CRSF frames.


## Acknowledgements

* The simplicity of this repo was motivating: https://github.com/stepinside/Arduino-CRSF
* https://github.com/ZZ-Cat/CRSFforArduino is a promising library.