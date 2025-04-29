#include "reconf_command.h"

bool ReconfCommand::parseIPAddress(const char* str, uint8_t* ip) {
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

void ReconfCommand::process(std::string_view args) {
  // Format: "reconf:ip:port"
  size_t colon1 = args.find(':');
  if (colon1 == std::string_view::npos) {
    Serial.println("Invalid reconf command format");
    return;
  }

  std::string_view ip_str = args.substr(0, colon1);
  args = args.substr(colon1 + 1);

  size_t colon2 = args.find(':');
  if (colon2 == std::string_view::npos) {
    Serial.println("Invalid reconf command format");
    return;
  }

  std::string_view port_str = args.substr(colon2 + 1);

  uint16_t new_port = atoi(port_str.data());
  uint8_t new_ip[4];

  if (parseIPAddress(ip_str.data(), new_ip) && new_port > 0 &&
      new_port <= 65535) {
    // Reconfigure the CH9120 with new target IP and port
    ch9121_.Reconfigure(new_ip, new_port);

    Serial.print("Reconfigured to ");
    Serial.print(ip_str.data());
    Serial.print(":");
    Serial.println(new_port);
  } else {
    Serial.println("Invalid IP or port format");
  }
}