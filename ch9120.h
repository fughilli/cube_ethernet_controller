#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

// CH9121 configuration structure for IP and UART settings
struct CH9121Config {
  uint8_t gateway[4];
  uint8_t subnet_mask[4];
  uint8_t local_ip[4];
  uint8_t target_ip[4];
  uint16_t local_port;
  uint16_t target_port;
  uint32_t baud_rate;
  uint8_t mode;  // Mode: 0x00 - TCP Server, 0x01 - TCP Client, 0x02 - UDP
                 // Server, 0x03 - UDP Client
};

class CH9121 {
 public:
  // Constructor that initializes with SoftwareSerial, configuration, and pin
  // numbers
  CH9121(HardwareSerial* uart, const CH9121Config& config, uint8_t rst_pin,
         uint8_t cfg_pin)
      : uart_(uart), config_(config), rst_pin_(rst_pin), cfg_pin_(cfg_pin) {}

  // Begin function to set pin modes and initialize UART communication
  void Begin() {
    pinMode(rst_pin_, OUTPUT);
    pinMode(cfg_pin_, OUTPUT);
    digitalWrite(rst_pin_, HIGH);  // Set RST pin high
    digitalWrite(cfg_pin_, HIGH);  // Set CFG pin high
    delay(500);
  }

  // Function to start the CH9121 configuration
  void StartConfiguration() {
    digitalWrite(cfg_pin_, LOW);
    digitalWrite(rst_pin_, HIGH);
    delay(500);

    // Set UART baud rate to the initial setup rate.
    uart_->begin(9600);
  }

  // Function to end the CH9121 configuration
  void EndConfiguration() {
    SendSimpleCommand(0x0D);  // Save settings
    delay(200);
    SendSimpleCommand(0x0E);  // Apply settings
    delay(200);
    SendSimpleCommand(0x5E);  // Exit config mode
    delay(500);
    digitalWrite(cfg_pin_, HIGH);  // Set CFG pin high

    // Set UART baud rate to match configuration
    uart_->begin(config_.baud_rate);
  }

  // Configures the device with settings in config_
  void Configure() {
    StartConfiguration();

    SetMode(config_.mode);
    SetLocalIP(config_.local_ip);
    SetSubnetMask(config_.subnet_mask);
    SetGateway(config_.gateway);
    SetTargetIP(config_.target_ip);
    SetLocalPort(config_.local_port);
    SetTargetPort(config_.target_port);
    SetBaudRate(config_.baud_rate);

    EndConfiguration();
  }

  // Reconfigures the target IP and port
  void Reconfigure(const uint8_t* new_target_ip, uint16_t new_target_port) {
    // Update the internal config
    memcpy(config_.target_ip, new_target_ip, 4);
    config_.target_port = new_target_port;

    // Apply the new configuration
    StartConfiguration();
    SetTargetIP(config_.target_ip);
    SetTargetPort(config_.target_port);
    EndConfiguration();
  }

  // Main function to handle RX/TX for debugging purposes
  void HandleRXTX() {
    while (true) {
      while (uart_->available()) {
        char ch = uart_->read();
        uart_->write(ch);  // Echo received data
      }
    }
  }

 private:
  HardwareSerial* uart_;
  CH9121Config config_;
  uint8_t rst_pin_;
  uint8_t cfg_pin_;
  uint8_t tx_buffer_[8] = {0x57, 0xAB};

  // Helper function to send a simple 3-byte command
  void SendSimpleCommand(uint8_t command) {
    tx_buffer_[2] = command;
    uart_->write(tx_buffer_, 3);
    delay(100);
  }

  // Helper function to send 4-byte commands (mode, etc.)
  void Send4ByteCommand(uint8_t data, uint8_t command) {
    tx_buffer_[2] = command;
    tx_buffer_[3] = data;
    uart_->write(tx_buffer_, 4);
    delay(100);
  }

  // Helper function to send 5-byte commands (port, etc.)
  void Send5ByteCommand(uint16_t data, uint8_t command) {
    tx_buffer_[2] = command;
    tx_buffer_[3] = data & 0xFF;
    tx_buffer_[4] = (data >> 8) & 0xFF;
    uart_->write(tx_buffer_, 5);
    delay(100);
  }

  // Helper function to send 7-byte commands (IP, gateway, subnet mask)
  void Send7ByteCommand(uint8_t data[], uint8_t command) {
    tx_buffer_[2] = command;
    for (int i = 0; i < 4; i++) {
      tx_buffer_[3 + i] = data[i];
    }
    uart_->write(tx_buffer_, 7);
    delay(100);
  }

  // Helper function to send baud rate configuration command
  void SendBaudRateCommand(uint32_t data, uint8_t command) {
    tx_buffer_[2] = command;
    tx_buffer_[3] = data & 0xFF;
    tx_buffer_[4] = (data >> 8) & 0xFF;
    tx_buffer_[5] = (data >> 16) & 0xFF;
    tx_buffer_[6] = (data >> 24) & 0xFF;
    uart_->write(tx_buffer_, 7);
    delay(100);
  }

  // Setters for various configuration parameters
  void SetMode(uint8_t mode) { Send4ByteCommand(mode, 0x10); }
  void SetLocalIP(uint8_t ip[]) { Send7ByteCommand(ip, 0x11); }
  void SetSubnetMask(uint8_t subnet[]) { Send7ByteCommand(subnet, 0x12); }
  void SetGateway(uint8_t gateway[]) { Send7ByteCommand(gateway, 0x13); }
  void SetTargetIP(uint8_t target_ip[]) { Send7ByteCommand(target_ip, 0x15); }
  void SetLocalPort(uint16_t port) { Send5ByteCommand(port, 0x14); }
  void SetTargetPort(uint16_t target_port) {
    Send5ByteCommand(target_port, 0x16);
  }
  void SetBaudRate(uint32_t baud_rate) { SendBaudRateCommand(baud_rate, 0x21); }
};
