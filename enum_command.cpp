#include "enum_command.h"
#include <Arduino.h>

void EnumCommand::process(std::string_view args) {
    if (on_enum) on_enum();
    uint8_t dip = pca_.readDIP();
    uart_->print("{\"type\":\"controller\",\"dip\":");
    uart_->print(dip);
    uart_->println("}");
}
