#include "lcd_command.h"
#include <charconv> // Include for std::from_chars
#include <system_error> // Include for std::errc

bool LcdCommand::parseCoordinates(const char* str, uint8_t& x, uint8_t& y) {
    // Find the first colon
    const char* first_colon = strchr(str, ':');
    if (!first_colon) return false;

    // Find the second colon
    const char* second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) return false;

    // Parse x coordinate
    char* end;
    long x_val = strtol(str, &end, 10);
    if (end != first_colon || x_val < 0 || x_val >= width_) return false;

    // Parse y coordinate
    long y_val = strtol(first_colon + 1, &end, 10);
    if (end != second_colon || y_val < 0 || y_val >= height_) return false;

    x = static_cast<uint8_t>(x_val);
    y = static_cast<uint8_t>(y_val);
    return true;
}

void LcdCommand::process(std::string_view args) {
    if (args == "clear") {
        lcd_.clear();
        if (on_clear) on_clear();
        return;
    }

    // Expected format: "x:y:text"

    // Find the first colon
    size_t first_colon_pos = args.find(':');
    if (first_colon_pos == std::string_view::npos) {
        // Invalid format
        return;
    }

    // Find the second colon
    size_t second_colon_pos = args.find(':', first_colon_pos + 1);
    if (second_colon_pos == std::string_view::npos) {
        // Invalid format
        return;
    }

    // Extract substrings for x, y, and text
    std::string_view x_sv = args.substr(0, first_colon_pos);
    std::string_view y_sv = args.substr(first_colon_pos + 1, second_colon_pos - (first_colon_pos + 1));
    std::string_view text_sv = args.substr(second_colon_pos + 1);

    // Parse x coordinate using std::from_chars
    uint8_t x;
    auto x_result = std::from_chars(x_sv.data(), x_sv.data() + x_sv.size(), x);
    if (x_result.ec != std::errc() || x_result.ptr != x_sv.data() + x_sv.size() || x >= width_) {
        // Parsing failed, invalid characters found, or out of bounds
        return;
    }

    // Parse y coordinate using std::from_chars
    uint8_t y;
    auto y_result = std::from_chars(y_sv.data(), y_sv.data() + y_sv.size(), y);
    if (y_result.ec != std::errc() || y_result.ptr != y_sv.data() + y_sv.size() || y >= height_) {
        // Parsing failed, invalid characters found, or out of bounds
        return;
    }

    lcd_.setCursor(x, y);
    for (char c : text_sv) {
        lcd_.write(c);
    }
}
