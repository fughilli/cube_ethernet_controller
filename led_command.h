#pragma once

#include <FastLED.h>

#include "base64.h"
#include "command.h"

__attribute__((packed)) struct RGBval {
  uint8_t r, g, b;
};

__attribute__((packed)) struct LedControl {
  uint16_t num_leds;
  RGBval data[];
};

union LedControlBuffer {
  LedControl control;
  uint8_t raw[4096];
};

class LedCommand : public Command {
 public:
  LedCommand(CRGB* leds, size_t num_leds)
      : Command("led"), leds_(leds), num_leds_(num_leds) {}

  void process(std::string_view args) override;

 private:
  CRGB* leds_;
  size_t num_leds_;
  LedControlBuffer led_control_buffer_;

  // Helper function to parse RGB values
  bool parse_rgb(std::string_view str, uint8_t& r, uint8_t& g, uint8_t& b);
};
