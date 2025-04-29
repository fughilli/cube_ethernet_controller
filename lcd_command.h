#pragma once

#include <LiquidCrystal_PCF8574.h>

#include <functional>
#include <string_view>

#include "command.h"

class LcdCommand : public Command {
 public:
  LcdCommand(LiquidCrystal_PCF8574& lcd, uint8_t width, uint8_t height)
      : Command("lcd"), lcd_(lcd), width_(width), height_(height) {}

  void process(std::string_view args) override;

  std::function<void()> on_clear;

 private:
  LiquidCrystal_PCF8574& lcd_;
  const uint8_t width_;
  const uint8_t height_;

  // Helper function to parse coordinates from string
  bool parseCoordinates(const char* str, uint8_t& x, uint8_t& y);
};