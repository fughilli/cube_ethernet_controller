#pragma once
#include <Wire.h>

class PCA9555 {
 public:
  static constexpr uint8_t I2C_ADDR = 0x20;
  static constexpr uint8_t NUM_BUTTONS = 6;
  static constexpr uint8_t NUM_LEDS = 6;

  PCA9555(TwoWire& wire = Wire) : wire_(wire) {}
  void begin();
  void setLed(uint8_t idx, bool on);
  void setAllLeds(const bool states[NUM_LEDS]);
  uint8_t readButtons();  // returns 6 LSBs as button states
  uint8_t readDIP();      // returns 4 LSBs as DIP value
  void updateOutputs();

 private:
  TwoWire& wire_;
  uint8_t led_state_ = 0;
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);
};
