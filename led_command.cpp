#include "led_command.h"

#include <stddef.h>
#include <stdio.h>

#include <algorithm>
#include <string_view>

bool LedCommand::parse_rgb(std::string_view str, uint8_t& r, uint8_t& g,
                           uint8_t& b) {
  // Format: "index r g b" or "all r g b"
  size_t space1 = str.find(' ');
  if (space1 == std::string_view::npos) return false;

  std::string_view index_str = str.substr(0, space1);
  str = str.substr(space1 + 1);

  size_t space2 = str.find(' ');
  if (space2 == std::string_view::npos) return false;

  std::string_view r_str = str.substr(0, space2);
  str = str.substr(space2 + 1);

  size_t space3 = str.find(' ');
  if (space3 == std::string_view::npos) return false;

  std::string_view g_str = str.substr(0, space3);
  std::string_view b_str = str.substr(space3 + 1);

  // Parse values
  int r_val = atoi(r_str.data());
  int g_val = atoi(g_str.data());
  int b_val = atoi(b_str.data());

  if (r_val < 0 || r_val > 255 || g_val < 0 || g_val > 255 || b_val < 0 ||
      b_val > 255) {
    return false;
  }

  r = r_val;
  g = g_val;
  b = b_val;

  return true;
}

void LedCommand::process(std::string_view args) {
  static uint32_t last_update = 0;
  static float avg_rate = 0.0f;
  const float alpha = 0.1f;  // Filter coefficient

  uint32_t now = millis();
  if (last_update > 0) {
    float dt = (now - last_update) / 1000.0f;
    if (dt > 0) {
      float instant_rate = 1.0f / dt;
      avg_rate = alpha * instant_rate + (1.0f - alpha) * avg_rate;
    }
  }
  last_update = now;

  if (Base64Decode(
          std::span<const uint8_t>((const uint8_t*)args.data(), args.size()),
          std::span(led_control_buffer_.raw,
                    sizeof(led_control_buffer_.raw)))) {
    auto& control = led_control_buffer_.control;
    for (int i = 0; i < num_leds_ && i < control.num_leds; ++i) {
      leds_[i] = CRGB(control.data[i].r, control.data[i].g, control.data[i].b);
    }
    FastLED.show();

    char tx_buf[128];
    int formatted =
        snprintf(tx_buf, 128, "up %d %.1f Hz\n", control.num_leds, avg_rate);
    Serial.write(tx_buf, formatted);
  } else {
    Serial.println("Failed to decode base64 LED data");
  }
}
