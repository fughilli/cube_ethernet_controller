#pragma once
#include <HardwareSerial.h>

#include <functional>
#include <string_view>

#include "command.h"
#include "pca9555.h"

class EnumCommand : public Command {
 public:
  EnumCommand(PCA9555& pca, HardwareSerial* uart)
      : Command("enum"), pca_(pca), uart_(uart) {}
  void process(std::string_view args) override;
  std::function<void()> on_enum;

 private:
  PCA9555& pca_;
  HardwareSerial* uart_;
};
