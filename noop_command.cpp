#include "noop_command.h"
#include <Arduino.h>

void NoopCommand::process(std::string_view args) {
    if (on_arm) {
        on_arm();
    }
}
