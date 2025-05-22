#pragma once
#include <functional>
#include <string_view>

#include "command.h"

class NoopCommand : public Command {
 public:
  NoopCommand() : Command("noop") {}
  void process(std::string_view args) override;

  std::function<void()> on_arm;
};
