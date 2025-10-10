#include "netconfig_command.h"

#include <Arduino.h>
#include <stdio.h>

bool NetConfigCommand::parseIPAddress(const char* str, uint8_t* ip) {
  int values[4];
  if (sscanf(str, "%d.%d.%d.%d", &values[0], &values[1], &values[2],
             &values[3]) != 4) {
    return false;
  }

  for (int i = 0; i < 4; i++) {
    if (values[i] < 0 || values[i] > 255) {
      return false;
    }
    ip[i] = values[i];
  }
  return true;
}

void NetConfigCommand::processGet() {
  const NetworkConfig& config = storage_.getConfig();

  char tx_buf[512];
  int formatted = snprintf(
      tx_buf, sizeof(tx_buf),
      "{\"base_ip\":\"%d.%d.%d.%d\","
      "\"subnet_mask\":\"%d.%d.%d.%d\","
      "\"gateway\":\"%d.%d.%d.%d\","
      "\"target_ip\":\"%d.%d.%d.%d\","
      "\"local_port\":%d,"
      "\"target_port\":%d,"
      "\"baud_rate\":%lu,"
      "\"mode\":%d}",
      config.base_ip[0], config.base_ip[1], config.base_ip[2],
      config.base_ip[3], config.subnet_mask[0], config.subnet_mask[1],
      config.subnet_mask[2], config.subnet_mask[3], config.gateway[0],
      config.gateway[1], config.gateway[2], config.gateway[3],
      config.target_ip[0], config.target_ip[1], config.target_ip[2],
      config.target_ip[3], config.local_port, config.target_port,
      config.baud_rate, config.mode);

  Serial2.write(tx_buf, formatted);
  Serial2.write('\n');
  Serial.println("Sent network config");
}

void NetConfigCommand::processSet(std::string_view args) {
  // Format: "set:field:value"
  // Supported fields: base_ip, subnet_mask, gateway, target_ip, local_port,
  // target_port

  size_t colon_pos = args.find(':');
  if (colon_pos == std::string_view::npos) {
    Serial.println("Invalid netconfig set format");
    Serial2.println("{\"error\":\"Invalid netconfig set format\"}");
    return;
  }

  std::string_view field = args.substr(0, colon_pos);
  std::string_view value = args.substr(colon_pos + 1);

  // Get current config
  NetworkConfig config = storage_.getConfig();
  bool needs_reconfig = false;

  // Copy value to null-terminated string for parsing
  char value_str[64];
  if (value.size() >= sizeof(value_str)) {
    Serial.println("Value too long");
    Serial2.println("{\"error\":\"Value too long\"}");
    return;
  }
  memcpy(value_str, value.data(), value.size());
  value_str[value.size()] = '\0';

  if (field == "base_ip") {
    if (!parseIPAddress(value_str, config.base_ip)) {
      Serial.println("Invalid IP address");
      Serial2.println("{\"error\":\"Invalid IP address\"}");
      return;
    }
    needs_reconfig = true;
  } else if (field == "subnet_mask") {
    if (!parseIPAddress(value_str, config.subnet_mask)) {
      Serial.println("Invalid subnet mask");
      Serial2.println("{\"error\":\"Invalid subnet mask\"}");
      return;
    }
    needs_reconfig = true;
  } else if (field == "gateway") {
    if (!parseIPAddress(value_str, config.gateway)) {
      Serial.println("Invalid gateway address");
      Serial2.println("{\"error\":\"Invalid gateway address\"}");
      return;
    }
    needs_reconfig = true;
  } else if (field == "target_ip") {
    if (!parseIPAddress(value_str, config.target_ip)) {
      Serial.println("Invalid target IP address");
      Serial2.println("{\"error\":\"Invalid target IP address\"}");
      return;
    }
    needs_reconfig = true;
  } else if (field == "local_port") {
    int port = atoi(value_str);
    if (port <= 0 || port > 65535) {
      Serial.println("Invalid local port");
      Serial2.println("{\"error\":\"Invalid local port\"}");
      return;
    }
    config.local_port = port;
    needs_reconfig = true;
  } else if (field == "target_port") {
    int port = atoi(value_str);
    if (port <= 0 || port > 65535) {
      Serial.println("Invalid target port");
      Serial2.println("{\"error\":\"Invalid target port\"}");
      return;
    }
    config.target_port = port;
    needs_reconfig = true;
  } else {
    Serial.print("Unknown field: ");
    Serial.println(field.data());
    Serial2.println("{\"error\":\"Unknown field\"}");
    return;
  }

  // Save configuration
  storage_.save(config);

  // Send confirmation
  Serial2.println("{\"status\":\"saved\"}");
  Serial.println("Network config updated");

  if (needs_reconfig) {
    Serial.println("Reconfiguration required - please reboot the controller");
    Serial2.println("{\"info\":\"Reboot required for changes to take effect\"}");
  }
}

void NetConfigCommand::processReset() {
  storage_.resetToDefaults();
  Serial2.println("{\"status\":\"reset\"}");
  Serial.println("Network config reset to defaults - reboot required");
  Serial2.println("{\"info\":\"Reboot required for changes to take effect\"}");
}

void NetConfigCommand::process(std::string_view args) {
  // Commands:
  // netconfig:get - Get current configuration
  // netconfig:set:field:value - Set a configuration field
  // netconfig:reset - Reset to defaults

  if (args.empty() || args == "get") {
    processGet();
  } else if (args.substr(0, 3) == "set") {
    std::string_view set_args = args.substr(3);
    if (!set_args.empty() && set_args[0] == ':') {
      set_args = set_args.substr(1);
    }
    processSet(set_args);
  } else if (args == "reset") {
    processReset();
  } else {
    Serial.println("Unknown netconfig command");
    Serial2.println("{\"error\":\"Unknown netconfig command\"}");
  }
}
