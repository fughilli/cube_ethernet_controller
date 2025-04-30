#include "backlight_command.h"
#include <cstring>

void BacklightCommand::process(std::string_view args) {
    bool states[PCA9555::NUM_LEDS] = {0};
    size_t idx = 0;
    size_t start = 0;
    while (idx < PCA9555::NUM_LEDS && start < args.size()) {
        size_t next = args.find(':', start);
        std::string_view token = (next == std::string_view::npos) ? args.substr(start) : args.substr(start, next - start);
        if (!token.empty() && (token[0] == '0' || token[0] == '1')) {
            states[idx] = (token[0] == '1');
            ++idx;
        }
        if (next == std::string_view::npos) break;
        start = next + 1;
    }
    pca_.setAllLeds(states);
}
