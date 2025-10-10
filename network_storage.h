#pragma once

#include <EEPROM.h>
#include <stdint.h>

// Network configuration structure
struct NetworkConfig {
  uint8_t base_ip[4];
  uint8_t subnet_mask[4];
  uint8_t gateway[4];
  uint8_t target_ip[4];
  uint16_t local_port;
  uint16_t target_port;
  uint32_t baud_rate;
  uint8_t mode;
};

class NetworkStorage {
 public:
  // Initialize EEPROM and load configuration
  void begin();

  // Load configuration from EEPROM
  // Returns true if valid configuration was loaded, false if defaults were used
  bool load(NetworkConfig& config);

  // Save configuration to EEPROM
  void save(const NetworkConfig& config);

  // Get current configuration
  const NetworkConfig& getConfig() const { return config_; }

  // Check if EEPROM has valid data
  bool isValid() const;

  // Reset to defaults
  void resetToDefaults();

 private:
  static constexpr uint16_t EEPROM_SIZE = 512;
  static constexpr uint16_t CONFIG_OFFSET = 0;
  static constexpr uint32_t MAGIC_NUMBER = 0xDEADBEEF;

  // EEPROM layout:
  // 0-3: Magic number (4 bytes)
  // 4-7: Checksum (4 bytes)
  // 8-31: NetworkConfig (24 bytes)

  NetworkConfig config_;

  // Calculate checksum for configuration
  uint32_t calculateChecksum(const NetworkConfig& config) const;

  // Get default configuration
  NetworkConfig getDefaultConfig() const;
};
