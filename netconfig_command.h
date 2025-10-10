#pragma once

#include <string_view>

#include "ch9120.h"
#include "command.h"
#include "network_storage.h"

class NetConfigCommand : public Command {
 public:
  NetConfigCommand(NetworkStorage& storage, CH9121& ch9121)
      : Command("netconfig"), storage_(storage), ch9121_(ch9121) {}

  void process(std::string_view args) override;

 private:
  NetworkStorage& storage_;
  CH9121& ch9121_;

  // Helper function to parse IP address from string
  bool parseIPAddress(const char* str, uint8_t* ip);

  // Process "get" command
  void processGet();

  // Process "set" command
  void processSet(std::string_view args);

  // Process "reset" command
  void processReset();
};
