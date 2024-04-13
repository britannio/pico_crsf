#include <stdio.h>
#include "pico/stdlib.h"
#include "crsf.h"

void on_rc_channels(const uint16_t channels[16]) {
  printf("Channel 1: %d\n", TICKS_TO_US(channels[0]));
  printf("Channel 2: %d\n", TICKS_TO_US(channels[1]));
  printf("Channel 3: %d\n", TICKS_TO_US(channels[2]));
  printf("Channel 4: %d\n", TICKS_TO_US(channels[3]));
  printf("Channel 5: %d\n", TICKS_TO_US(channels[4]));
  printf("Channel 6: %d\n", TICKS_TO_US(channels[5]));
  printf("Channel 7: %d\n", TICKS_TO_US(channels[6]));
  printf("Channel 8: %d\n", TICKS_TO_US(channels[7]));
}

void on_link_stats(const link_statistics_t *link_stats) {
  printf("RSSI: %d\n", link_stats->rssi);
  printf("Link Quality: %d\n", link_stats->link_quality);
  printf("SNR: %d\n", link_stats->snr);
  printf("TX Power: %d\n", link_stats->tx_power);
}

void on_failsafe(const bool failsafe) {
  printf("Failsafe: %d\n", failsafe);
}

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

void set_battery() {
  crsf_telem_set_battery_data(0, 0, 0, 0);
}
