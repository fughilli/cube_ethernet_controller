#include "ch9120.h"
#include <FastLED.h>
#include <span>
#include "base64.h"
#include <SoftwareSerial.h>
#include <algorithm>

const CH9121Config config = {
  .gateway = { 192, 168, 0, 1 },
  .subnet_mask = { 255, 255, 0, 0 },
  .local_ip = { 192, 168, 0, 51 },
  .target_ip = { 192, 168, 0, 1 },
  .local_port = 5000,
  .target_port = 4000,
  .baud_rate = 921600,
  .mode = 0x02,
};

CH9121<19, 18> ch9121(&Serial2, config);

std::array<uint8_t, 4096> rx_buffer;

__attribute__((packed)) struct RGBval {
  uint8_t r, g, b;
};

__attribute__((packed)) struct LedControl {
  uint16_t num_leds;
  RGBval data[];
};

union LedControlBuffer {
  LedControl control;
  uint8_t raw[4096];
};

LedControlBuffer led_control_buffer;

#define NUM_PIXELS 1
#define DATA_PIN 25

CRGB leds[NUM_PIXELS];

void updateControl(std::span<const uint8_t> data) {
  static uint32_t last_update = 0;
  static float avg_rate = 0.0f;
  const float alpha = 0.1f; // Filter coefficient

  uint32_t now = millis();
  if (last_update > 0) {
    float dt = (now - last_update) / 1000.0f;
    if (dt > 0) {
      float instant_rate = 1.0f / dt;
      avg_rate = alpha * instant_rate + (1.0f - alpha) * avg_rate;
    }
  }
  last_update = now;

  if (Base64Decode(data, std::span(led_control_buffer.raw, sizeof(led_control_buffer.raw)))) {
    auto& control = led_control_buffer.control;
    for (int i = 0; i < NUM_PIXELS && i < control.num_leds; ++i) {
      leds[i] = CRGB(control.data[i].r, control.data[i].g, control.data[i].b);
    }
    FastLED.show();
    char tx_buf[128];
    int formatted = snprintf(tx_buf, 128, "up %d %.1f Hz\n", control.num_leds, avg_rate);
    Serial.write(tx_buf, formatted);

    // Print the raw buffer
    // char hex_buf[8];
    // Serial.print("Raw[");
    // Serial.print(control.num_leds);
    // Serial.print("]: ");
    // for (int i = 0; i < std::min<int>(control.num_leds, NUM_PIXELS) * 3 + 2; ++i) {
    //   snprintf(hex_buf, 8, "%02X ", led_control_buffer.raw[i]);
    //   Serial.print(hex_buf);
    // }
    // Serial.println();
  } else {
    Serial.println("Failed to decode");
  }
}

void setup() {
  Serial.begin(921600);

  // Give some time for the serial port to be ready
  delay(1000);

  Serial.println("Initializing...");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_PIXELS);
  // put your setup code here, to run once:
  Serial2.setTX(20);
  Serial2.setRX(21);
  Serial2.setFIFOSize(1024);

  Serial.println("Starting CH9121 config...");
  ch9121.Begin();
  ch9121.Configure();
  Serial.println("Finished CH9121 config");
}

size_t rx_buffer_pos = 0;

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial2.available()) {
    char value = Serial2.read();
    Serial.print((int)value);
    if (value == '\n') {
      // Serial.print("Base64: ");
      // Serial.write(rx_buffer.data(), rx_buffer_pos);
      // Serial.println();
      updateControl({ rx_buffer.data(), rx_buffer_pos });
      rx_buffer_pos = 0;
    } else if (value == '?') {
      char tx_buf[128];
      int formatted = snprintf(tx_buf, 128, "{\"geom\": \"linear\", \"num_leds\": %d}", NUM_PIXELS);
      Serial2.write(tx_buf, formatted);
    } else if (rx_buffer_pos < rx_buffer.size()) {
      rx_buffer[rx_buffer_pos++] = value;
    }
  }
}
