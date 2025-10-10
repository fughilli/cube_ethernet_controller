#include "network_storage.h"
#include <Arduino.h>

void NetworkStorage::begin() {
  EEPROM.begin(EEPROM_SIZE);

  // Try to load configuration
  if (!load(config_)) {
    Serial.println("No valid network config found, using defaults");
    config_ = getDefaultConfig();
    save(config_);
  } else {
    Serial.println("Loaded network config from EEPROM");
  }
}

bool NetworkStorage::load(NetworkConfig& config) {
  // Read magic number
  uint32_t magic = 0;
  for (int i = 0; i < 4; i++) {
    magic |= ((uint32_t)EEPROM.read(CONFIG_OFFSET + i)) << (i * 8);
  }

  if (magic != MAGIC_NUMBER) {
    Serial.println("Invalid magic number in EEPROM");
    return false;
  }

  // Read stored checksum
  uint32_t stored_checksum = 0;
  for (int i = 0; i < 4; i++) {
    stored_checksum |= ((uint32_t)EEPROM.read(CONFIG_OFFSET + 4 + i)) << (i * 8);
  }

  // Read configuration data
  uint8_t* config_bytes = reinterpret_cast<uint8_t*>(&config);
  for (size_t i = 0; i < sizeof(NetworkConfig); i++) {
    config_bytes[i] = EEPROM.read(CONFIG_OFFSET + 8 + i);
  }

  // Verify checksum
  uint32_t calculated_checksum = calculateChecksum(config);
  if (calculated_checksum != stored_checksum) {
    Serial.print("Checksum mismatch: stored=");
    Serial.print(stored_checksum, HEX);
    Serial.print(", calculated=");
    Serial.println(calculated_checksum, HEX);
    return false;
  }

  return true;
}

void NetworkStorage::save(const NetworkConfig& config) {
  // Write magic number
  uint32_t magic = MAGIC_NUMBER;
  for (int i = 0; i < 4; i++) {
    EEPROM.write(CONFIG_OFFSET + i, (magic >> (i * 8)) & 0xFF);
  }

  // Calculate and write checksum
  uint32_t checksum = calculateChecksum(config);
  for (int i = 0; i < 4; i++) {
    EEPROM.write(CONFIG_OFFSET + 4 + i, (checksum >> (i * 8)) & 0xFF);
  }

  // Write configuration data
  const uint8_t* config_bytes = reinterpret_cast<const uint8_t*>(&config);
  for (size_t i = 0; i < sizeof(NetworkConfig); i++) {
    EEPROM.write(CONFIG_OFFSET + 8 + i, config_bytes[i]);
  }

  EEPROM.commit();

  Serial.println("Network config saved to EEPROM");
}

bool NetworkStorage::isValid() const {
  NetworkConfig temp_config;
  return const_cast<NetworkStorage*>(this)->load(temp_config);
}

void NetworkStorage::resetToDefaults() {
  config_ = getDefaultConfig();
  save(config_);
  Serial.println("Network config reset to defaults");
}

uint32_t NetworkStorage::calculateChecksum(const NetworkConfig& config) const {
  uint32_t checksum = 0;
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&config);

  // Simple checksum: sum all bytes
  for (size_t i = 0; i < sizeof(NetworkConfig); i++) {
    checksum += bytes[i];
    // Rotate left by 1 bit to mix the bits
    checksum = (checksum << 1) | (checksum >> 31);
  }

  return checksum;
}

NetworkConfig NetworkStorage::getDefaultConfig() const {
  NetworkConfig defaults;

  // Default base IP: 2.0.99.150
  defaults.base_ip[0] = 2;
  defaults.base_ip[1] = 0;
  defaults.base_ip[2] = 99;
  defaults.base_ip[3] = 150;

  // Default subnet mask: 255.255.0.0
  defaults.subnet_mask[0] = 255;
  defaults.subnet_mask[1] = 255;
  defaults.subnet_mask[2] = 0;
  defaults.subnet_mask[3] = 0;

  // Default gateway: 2.0.99.189
  defaults.gateway[0] = 2;
  defaults.gateway[1] = 0;
  defaults.gateway[2] = 99;
  defaults.gateway[3] = 189;

  // Default target IP: 2.0.99.189
  defaults.target_ip[0] = 2;
  defaults.target_ip[1] = 0;
  defaults.target_ip[2] = 99;
  defaults.target_ip[3] = 189;

  defaults.local_port = 51333;
  defaults.target_port = 51334;
  defaults.baud_rate = 921600;
  defaults.mode = 0x00;  // TCP Server mode

  return defaults;
}
