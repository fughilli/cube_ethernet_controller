#include <FastLED.h>
#include <LiquidCrystal_PCF8574.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include <algorithm>
#include <span>

#include "backlight_command.h"
#include "base64.h"
#include "ch9120.h"
#include "command.h"
#include "config_command.h"
#include "enum_command.h"
#include "lcd_command.h"
#include "led_command.h"
#include "netconfig_command.h"
#include "network_storage.h"
#include "noop_command.h"
#include "pca9555.h"
#include "reconf_command.h"
#include "reset_command.h"

const uint8_t INT_PIN = 2;  // GP2 on RP2040

// Network storage for persistent configuration
NetworkStorage network_storage;

CH9121Config config;

// Create CH9121 instance with pin numbers
CH9121 ch9121(&Serial2, config, 19, 18);

#define NUM_PIXELS 1
#define DATA_PIN 25

CRGB leds[NUM_PIXELS];

// LCD dimensions
const uint8_t LCD_WIDTH = 20;
const uint8_t LCD_HEIGHT = 4;

// Initialize LCD display
LiquidCrystal_PCF8574 lcd(0x27);

PCA9555 pca;
BacklightCommand backlight_command(pca);
EnumCommand enum_command(pca, &Serial2);

// Create commands
LedCommand led_command(leds, NUM_PIXELS);
ConfigCommand config_command(NUM_PIXELS);
ReconfCommand reconf_command(ch9121);
LcdCommand lcd_command(lcd, LCD_WIDTH, LCD_HEIGHT);
ResetCommand reset_command;
NoopCommand noop_command;
NetConfigCommand netconfig_command(network_storage, ch9121);
Command* commands[] = {&led_command,      &config_command, &reconf_command,
                       &lcd_command,      &backlight_command, &enum_command,
                       &reset_command,    &noop_command,   &netconfig_command};
CommandProcessor command_processor(commands);

volatile bool button_int_flag = false;
uint8_t last_button_state = 0;
uint8_t debounced_button_state = 0;
uint32_t last_button_change_times[PCA9555::NUM_BUTTONS] = {0};
const uint32_t DEBOUNCE_DELAY = 50;  // 50ms debounce delay

// Debug/boot message state
uint8_t boot_dip = 0;
uint16_t enum_count = 0;
bool debug_message_enabled = true;

// Heartbeat and watchdog timing
const uint32_t HEARTBEAT_INTERVAL = 500;  // 500ms between heartbeats
const uint32_t WATCHDOG_TIMEOUT = 5000;   // 2 second timeout
uint32_t last_heartbeat = 0;
uint32_t last_command_time = 0;
bool heartbeat_armed = false;  // Flag to track if heartbeat is active

void onButtonInt() { button_int_flag = true; }

void show_boot_message() {
  if (!debug_message_enabled) {
    return;
  }
  uint8_t buttons = pca.readButtons();
  lcd.setCursor(0, 0);
  lcd.print("Controller ");
  lcd.print(boot_dip);
  lcd.setCursor(0, 1);
  lcd.print("Enum ");
  lcd.print(enum_count);
  lcd.setCursor(0, 2);
  lcd.print("I/O dbg: [");
  for (uint8_t i = 0; i < PCA9555::NUM_BUTTONS; ++i) {
    lcd.print((buttons >> i) & 1);
  }
  lcd.print("]");
  lcd.setCursor(0, 3);
  lcd.print("IP: ");
  lcd.print(config.local_ip[0]);
  lcd.print(".");
  lcd.print(config.local_ip[1]);
  lcd.print(".");
  lcd.print(config.local_ip[2]);
  lcd.print(".");
  lcd.print(config.local_ip[3]);
}

void setup() {
  Serial.begin(921600);

  // Give some time for the serial port to be ready
  delay(1000);

  Serial.println("Initializing...");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_PIXELS);

  // Initialize I2C and LCD
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.setClock(400000);
  Wire.begin();

  pinMode(28, OUTPUT);
  digitalWrite(28, HIGH);

  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Initializing...");

  // put your setup code here, to run once:
  Serial2.setTX(20);
  Serial2.setRX(21);
  Serial2.setFIFOSize(65536);

  // PCA9555 setup
  pca.begin();

  // Initialize network storage and load configuration
  network_storage.begin();
  const NetworkConfig& net_config = network_storage.getConfig();

  // Convert NetworkConfig to CH9121Config and apply DIP offset
  boot_dip = pca.readDIP();

  memcpy(config.gateway, net_config.gateway, 4);
  memcpy(config.subnet_mask, net_config.subnet_mask, 4);
  memcpy(config.target_ip, net_config.target_ip, 4);

  // Set local IP from base IP + DIP offset
  memcpy(config.local_ip, net_config.base_ip, 4);
  config.local_ip[3] = net_config.base_ip[3] + (boot_dip & 0x0F);

  config.local_port = net_config.local_port;
  config.target_port = net_config.target_port;
  config.baud_rate = net_config.baud_rate;
  config.mode = net_config.mode;

  ch9121 = CH9121(&Serial2, config, 19, 18);  // re-init with new config

  Serial.print("DIP switch value: ");
  Serial.println(boot_dip);
  Serial.print("Setting IP to: ");
  Serial.print(config.local_ip[0]);
  Serial.print(".");
  Serial.print(config.local_ip[1]);
  Serial.print(".");
  Serial.print(config.local_ip[2]);
  Serial.print(".");
  Serial.println(config.local_ip[3]);

  Serial.println("Starting CH9121 config...");
  ch9121.Begin();
  delay(1000);  // Give more time for hardware initialization
  ch9121.Configure();
  delay(1000);  // Give more time for configuration to take effect
  Serial.println("Finished CH9121 config");

  // Clear LCD and show ready message
  lcd.clear();
  lcd.print("Ready");

  // INT pin setup
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), onButtonInt, FALLING);
  last_button_state = pca.readButtons();

  enum_count = 0;
  debug_message_enabled = true;
  show_boot_message();

  enum_command.on_enum = []() {
    ++enum_count;
    show_boot_message();
    Serial.print("Enumeration request received. Count: ");
    Serial.println(enum_count);
  };
  lcd_command.on_clear = []() { debug_message_enabled = false; };

  noop_command.on_arm = []() {
    if (!heartbeat_armed) {
      heartbeat_armed = true;
      Serial.println("Heartbeat mechanism armed");
    }
  };

  command_processor.on_command = [](std::string_view command_line) {
    Serial.print("Received command: \"");
    for (char c : command_line) {
        Serial.write(c);
    }
    Serial.println("\"");
  };

  last_heartbeat = millis();
  last_command_time = millis();
  heartbeat_armed = false;  // Start with heartbeat disabled

  rp2040.wdt_begin(1000);  // 1000 ms timeout
}

void maybe_reset_sequence(uint8_t r_buttons) {
  // Detect a sequence of:
  // Holding buttons 0, 2 for 2 seconds
  // Holding buttons 0, 2, 5 for 2 seconds
  //
  // If this sequence is detected, reset the controller.
  static uint32_t sequence_state = 0;
  static uint32_t sequence_start_time_ms = 0;

  switch (sequence_state) {
    case 0:
      if (r_buttons == 0x05) {
        sequence_state = 1;
        sequence_start_time_ms = millis();
        Serial.println("Sequence 1 started");
      }
      break;
    case 1:
      if (r_buttons != 0x05) {
        sequence_state = 0;
        Serial.println("Sequence 1 ended");
      } else if (millis() - sequence_start_time_ms >= 2000) {
        sequence_state = 2;
        sequence_start_time_ms = millis();
        Serial.println("Sequence 2 started");
      }
      break;
    case 2:
      if (r_buttons & 0x05 != 0x05) {
        sequence_state = 0;
        Serial.println("Sequence 2 ended");
      } else if (r_buttons == 0x15) {
        sequence_state = 3;
        sequence_start_time_ms = millis();
      }
      break;
    case 3:
      if (r_buttons != 0x15) {
        sequence_state = 0;
        Serial.println("Sequence 3 ended");
      } else if (millis() - sequence_start_time_ms >= 2000) {
        Serial.println("Resetting controller via buttons");
        rp2040.reboot();
      }
      break;
  }
}

void loop() {
  // Pet watchdog at beginning of loop to reduce likelihood of reset
  // on first iteration.
  rp2040.wdt_reset();

  uint32_t current_time = millis();

  // Only check watchdog and send heartbeats if armed
  if (heartbeat_armed) {
    // Check for host command timeout
    if (current_time - last_command_time >= WATCHDOG_TIMEOUT) {
      Serial.println("Host command timeout - resetting");
      rp2040.reboot();
    }

    // Send heartbeat
    if (current_time - last_heartbeat >= HEARTBEAT_INTERVAL) {
      Serial2.println("{\"type\":\"heartbeat\"}");
      last_heartbeat = current_time;
    }
  }

  // Process incoming commands
  if (Serial2.available()) {
    char value = Serial2.read();
    command_processor.process_char(value);
    last_command_time =
        current_time;  // Update last command time on any command
  }

  uint8_t state = pca.readButtons();
  bool state_changed = false;

  // Check each button independently
  for (uint8_t i = 0; i < PCA9555::NUM_BUTTONS; ++i) {
    bool current_button = (state >> i) & 1;
    bool last_button = (last_button_state >> i) & 1;
    bool debounced_button = (debounced_button_state >> i) & 1;

    // If button state changed, update its last change time
    if (current_button != last_button) {
      last_button_change_times[i] = current_time;
    }

    // If enough time has passed since last change, update the debounced state
    if ((current_time - last_button_change_times[i]) >= DEBOUNCE_DELAY) {
      if (current_button != debounced_button) {
        // Update the specific bit in debounced_button_state
        if (current_button) {
          debounced_button_state |= (1 << i);
        } else {
          debounced_button_state &= ~(1 << i);
        }
        state_changed = true;
      }
    }
  }

  // Update last_button_state
  last_button_state = state;

  maybe_reset_sequence(debounced_button_state);

  // Only send update if any button state changed
  if (state_changed) {
    Serial2.print("{\"buttons\":[");
    for (uint8_t i = 0; i < PCA9555::NUM_BUTTONS; ++i) {
      Serial2.print((debounced_button_state >> i) & 1);
      if (i < PCA9555::NUM_BUTTONS - 1) Serial2.print(",");
    }
    Serial2.println("]}");
    show_boot_message();
  }
}
