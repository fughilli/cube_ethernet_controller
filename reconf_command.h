#pragma once

#include <string_view>

#include "ch9120.h"
#include "command.h"

class ReconfCommand : public Command {
 public:
  ReconfCommand(CH9121& ch9121) : Command("reconf"), ch9121_(ch9121) {}

  void process(std::string_view args) override;

 private:
  CH9121& ch9121_;

  // Helper function to parse IP address from string
  bool parseIPAddress(const char* str, uint8_t* ip);
};