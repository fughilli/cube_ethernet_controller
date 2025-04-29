#include "command.h"

#include <Arduino.h>
#include <stdio.h>

#include <algorithm>
#include <string_view>

void CommandProcessor::process_char(char c) {
  if (c == '\n') {
    // Process the complete command
    process_command(std::string_view(buffer_.data(), buffer_pos_));
    buffer_pos_ = 0;
  } else if (buffer_pos_ < buffer_.size()) {
    buffer_[buffer_pos_++] = c;
  }
}

void CommandProcessor::process_command(std::string_view line) {
  // Find the first space to separate command from args
  size_t space_pos = line.find(' ');
  std::string_view prefix = line.substr(0, space_pos);
  std::string_view args = space_pos != std::string_view::npos
                              ? line.substr(space_pos + 1)
                              : std::string_view();

  // Find matching command
  for (Command* cmd : commands_) {
    if (cmd->prefix() == prefix) {
      cmd->process(args);
      return;
    }
  }

  // No matching command found
  Serial.print("Unknown command: ");
  Serial.println(prefix.data());
}