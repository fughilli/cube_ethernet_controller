#include "base64.h"

#include <stdint.h>

#include <span>

constexpr int kDecodeTable[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0-15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 16-31
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  // 32-47
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, 0,  -1, -1,  // 48-63
    -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,  // 64-79
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  // 80-95
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 96-111
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,  // 112-127
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 128-143
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 144-159
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 160-175
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 176-191
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 192-207
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 208-223
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 224-239
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1   // 240-255
};

bool Base64Decode(std::span<const uint8_t> base64, std::span<uint8_t> out) {
  size_t outIndex = 0;
  int buffer = 0;
  int bitsLeft = 0;

  for (const uint8_t& byte : base64) {
    if (byte > 255 || kDecodeTable[byte] == -1) {
      if (byte == '=' || byte == '\n' || byte == '\r' || byte == ' ') {
        continue;  // Skip padding or whitespace
      } else {
        return false;  // Invalid character
      }
    }

    buffer = (buffer << 6) | kDecodeTable[byte];
    bitsLeft += 6;

    if (bitsLeft >= 8) {
      bitsLeft -= 8;
      uint8_t decodedByte = (buffer >> bitsLeft) & 0xFF;
      if (outIndex >= out.size()) {
        return false;  // Output buffer too small
      }
      out[outIndex++] = decodedByte;
    }
  }

  // Check for incomplete byte at the end
  if (bitsLeft > 0 && (buffer & ((1 << bitsLeft) - 1)) != 0) {
    return false;  // Trailing bits are not zero, invalid padding
  }

  return true;
}