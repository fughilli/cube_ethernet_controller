#pragma once

#include <array>
#include <span>
#include <string_view>

class Command {
 public:
  explicit Command(std::string_view prefix) : prefix_(prefix) {}
  virtual ~Command() = default;

  // Process the command arguments
  virtual void process(std::string_view args) = 0;

  // Get the command prefix
  std::string_view prefix() const { return prefix_; }

 private:
  std::string_view prefix_;
};

class CommandProcessor {
 public:
  // Constructor takes a span of commands to process
  explicit CommandProcessor(std::span<Command*> commands)
      : commands_(commands) {}

  // Process a single character
  void process_char(char c);

 private:
  std::span<Command*> commands_;
  std::array<char, 256> buffer_;
  size_t buffer_pos_ = 0;

  // Process a complete command line
  void process_command(std::string_view line);
};
