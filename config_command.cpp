#include "config_command.h"

#include <Arduino.h>
#include <stdio.h>

#include <string_view>

void ConfigCommand::process(std::string_view args) {
  char tx_buf[128];
  int formatted = snprintf(
      tx_buf, 128, "{\"geom\": \"linear\", \"num_leds\": %d}", num_leds_);
  Serial2.write(tx_buf, formatted);
}