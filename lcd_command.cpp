#include "lcd_command.h"
#include <cstdlib>

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
    uint8_t x, y;
    if (!parseCoordinates(args.data(), x, y)) {
        return;
    }
    
    // Extract the text to display (everything after the second colon)
    const char* text_start = strchr(strchr(args.data(), ':') + 1, ':') + 1;
    if (!text_start) return;
    
    // Set cursor position and print text
    lcd_.setCursor(x, y);
    lcd_.print(text_start);
} 