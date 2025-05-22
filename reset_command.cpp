#include "reset_command.h"
#include <Arduino.h>

void ResetCommand::process(std::string_view args) {
    // Small delay to ensure the response is sent
    delay(100);
    // Reset the controller
    rp2040.reboot();
}
