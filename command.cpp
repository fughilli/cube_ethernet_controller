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
  // Find matching command by checking prefix substring
  for (Command* cmd : commands_) {
    std::string_view cmd_prefix = cmd->prefix();
    if (line.size() >= cmd_prefix.size() &&
        line.substr(0, cmd_prefix.size()) == cmd_prefix) {
      // Found matching command, extract args after prefix
      std::string_view args;
      if (line.size() > cmd_prefix.size()) {
        if (line[cmd_prefix.size()] == ':') {
          args = line.substr(cmd_prefix.size() + 1);
        } else {
          // Prefix matched but not at a word boundary
          continue;
        }
      }
      cmd->process(args);
      return;
    }
  }

  // No matching command found
  Serial.write("Unknown command: ", 16);
  for (char c : line) {
    Serial.write(c);
  }
  Serial.write('\n');
}
