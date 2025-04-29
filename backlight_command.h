#pragma once
#include <string_view>

#include "command.h"
#include "pca9555.h"

class BacklightCommand : public Command {
 public:
  BacklightCommand(PCA9555& pca) : Command("backlight"), pca_(pca) {}
  void process(std::string_view args) override;

 private:
  PCA9555& pca_;
};