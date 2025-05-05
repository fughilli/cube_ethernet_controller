#include "pca9555.h"

// PCA9555 register addresses
constexpr uint8_t REG_INPUT_0 = 0x00;
constexpr uint8_t REG_INPUT_1 = 0x01;
constexpr uint8_t REG_OUTPUT_0 = 0x02;
constexpr uint8_t REG_OUTPUT_1 = 0x03;
constexpr uint8_t REG_CONFIG_0 = 0x06;
constexpr uint8_t REG_CONFIG_1 = 0x07;

void PCA9555::begin() {
    // IO0_0:5 = LED outputs (0=output), IO0_6:7 = unused (set as input)
    // IO1_0:5 = BUTTON inputs (1=input), IO1_4:7 = DIP inputs (1=input)
    // IO1_6:7 = DIP inputs (1=input)
    writeRegister(REG_CONFIG_0, 0b10101010); // LED_x as output, BUTTON_x as input
    writeRegister(REG_CONFIG_1, 0b11111010); // LED_4/5 as output, BUTTON_4/5 and DIP as input
    // Set all LEDs OFF
    writeRegister(REG_OUTPUT_0, 0x00);
    writeRegister(REG_OUTPUT_1, 0x00);
}

void PCA9555::setLed(uint8_t idx, bool on) {
    if (idx >= NUM_LEDS) return;
    uint8_t out0 = readRegister(REG_OUTPUT_0);
    uint8_t out1 = readRegister(REG_OUTPUT_1);
    switch(idx) {
        case 0: if (on) out0 |= (1 << 0); else out0 &= ~(1 << 0); break;
        case 1: if (on) out0 |= (1 << 2); else out0 &= ~(1 << 2); break;
        case 2: if (on) out0 |= (1 << 4); else out0 &= ~(1 << 4); break;
        case 3: if (on) out0 |= (1 << 6); else out0 &= ~(1 << 6); break;
        case 4: if (on) out1 |= (1 << 0); else out1 &= ~(1 << 0); break;
        case 5: if (on) out1 |= (1 << 2); else out1 &= ~(1 << 2); break;
    }
    writeRegister(REG_OUTPUT_0, out0);
    writeRegister(REG_OUTPUT_1, out1);
}

void PCA9555::setAllLeds(const bool states[NUM_LEDS]) {
    uint8_t out0 = 0, out1 = 0;
    if (states[0]) out0 |= (1 << 0);
    if (states[1]) out0 |= (1 << 2);
    if (states[2]) out0 |= (1 << 4);
    if (states[3]) out0 |= (1 << 6);
    if (states[4]) out1 |= (1 << 0);
    if (states[5]) out1 |= (1 << 2);
    writeRegister(REG_OUTPUT_0, out0);
    writeRegister(REG_OUTPUT_1, out1);
}

uint8_t PCA9555::readButtons() {
    uint8_t in0 = readRegister(REG_INPUT_0);
    uint8_t in1 = readRegister(REG_INPUT_1);
    uint8_t result = 0;
    if (!(in0 & (1 << 1))) result |= (1 << 0); // BUTTON_0
    if (!(in0 & (1 << 3))) result |= (1 << 1); // BUTTON_1
    if (!(in0 & (1 << 5))) result |= (1 << 2); // BUTTON_2
    if (!(in0 & (1 << 7))) result |= (1 << 3); // BUTTON_3
    if (!(in1 & (1 << 1))) result |= (1 << 4); // BUTTON_4
    if (!(in1 & (1 << 3))) result |= (1 << 5); // BUTTON_5
    return result;
}

uint8_t PCA9555::readDIP() {
    uint8_t in1 = readRegister(REG_INPUT_1);
    return (in1 >> 4) & 0x0F;
}

void PCA9555::updateOutputs() {
    // Not used in new logic, but could be implemented if needed
}

void PCA9555::writeRegister(uint8_t reg, uint8_t value) {
    wire_.beginTransmission(I2C_ADDR);
    wire_.write(reg);
    wire_.write(value);
    wire_.endTransmission();
}

uint8_t PCA9555::readRegister(uint8_t reg) {
    wire_.beginTransmission(I2C_ADDR);
    wire_.write(reg);
    wire_.endTransmission(false);
    wire_.requestFrom(I2C_ADDR, (uint8_t)1);
    if (wire_.available()) {
        return wire_.read();
    }
    return 0xFF;
}
