#pragma once
#include <string_view>

#include "command.h"

class ResetCommand : public Command {
 public:
  ResetCommand() : Command("reset") {}
  void process(std::string_view args) override;
};
