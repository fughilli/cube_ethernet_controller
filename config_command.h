#pragma once

#include <stddef.h>

#include <string_view>

#include "command.h"

class ConfigCommand : public Command {
 public:
  ConfigCommand(size_t num_leds) : Command("?"), num_leds_(num_leds) {}

  void process(std::string_view args) override;

 private:
  size_t num_leds_;
};
