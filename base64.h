#pragma once

#include <stdint.h>

#include <span>

bool Base64Decode(std::span<const uint8_t> base64, std::span<uint8_t> out);