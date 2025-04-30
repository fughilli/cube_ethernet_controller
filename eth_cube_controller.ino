#include <FastLED.h>
#include <SoftwareSerial.h>

#include <algorithm>
#include <span>

#include "base64.h"
#include "ch9120.h"
#include "command.h"
#include "config_command.h"
#include "led_command.h"
#include "reconf_command.h"
#include "lcd_command.h"
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include "pca9555.h"
#include "backlight_command.h"
#include "enum_command.h"

const uint8_t INT_PIN = 2; // GP2 on RP2040

const CH9121Config default_config = {
    .gateway = {192, 168, 0, 1},
    .subnet_mask = {255, 255, 255, 0},
    .local_ip = {192, 168, 0, 51}, // will be overwritten by DIP
    .target_ip = {192, 168, 0, 1},
    .local_port = 5000,
    .target_port = 5000,
    .baud_rate = 921600,
    .mode = 0x02,
};

CH9121Config config = default_config;

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
Command* commands[] = {&led_command, &config_command, &reconf_command, &lcd_command, &backlight_command, &enum_command};
CommandProcessor command_processor(commands);

volatile bool button_int_flag = false;
uint8_t last_button_state = 0;

// Debug/boot message state
uint8_t boot_dip = 0;
uint16_t enum_count = 0;
bool debug_message_enabled = true;

void onButtonInt() {
  button_int_flag = true;
}

void show_boot_message() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Controller ");
  lcd.print(boot_dip);
  lcd.setCursor(0, 1);
  lcd.print("Enum ");
  lcd.print(enum_count);
}

void setup() {
  Serial.begin(921600);

  // Give some time for the serial port to be ready
  delay(1000);

  Serial.println("Initializing...");

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_PIXELS);

  // Initialize I2C and LCD
  Wire.begin();
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  lcd.print("Initializing...");

  // put your setup code here, to run once:
  Serial2.setTX(20);
  Serial2.setRX(21);
  Serial2.setFIFOSize(1024);

  // PCA9555 setup
  pca.begin();

  // Read DIP and set CH9121 IP
  boot_dip = pca.readDIP();
  config.local_ip[3] = 50 + (boot_dip & 0x0F);
  ch9121 = CH9121(&Serial2, config, 19, 18); // re-init with new config

  Serial.print("DIP switch value: ");
  Serial.println(boot_dip);
  Serial.print("Setting IP to: 192.168.0.");
  Serial.println(config.local_ip[3]);

  Serial.println("Starting CH9121 config...");
  ch9121.Begin();
  delay(1000); // Give more time for hardware initialization
  ch9121.Configure();
  delay(1000); // Give more time for configuration to take effect
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
    if (debug_message_enabled) {
      ++enum_count;
      show_boot_message();
      Serial.print("Enumeration request received. Count: ");
      Serial.println(enum_count);
    }
  };
  lcd_command.on_clear = []() {
    debug_message_enabled = false;
  };
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial2.available()) {
    char value = Serial2.read();
    command_processor.process_char(value);
  }
  if (button_int_flag) {
    button_int_flag = false;
    uint8_t state = pca.readButtons();
    if (state != last_button_state) {
      Serial.print("{\"buttons\":[");
      for (uint8_t i = 0; i < PCA9555::NUM_BUTTONS; ++i) {
        Serial.print((state >> i) & 1);
        if (i < PCA9555::NUM_BUTTONS - 1) Serial.print(",");
      }
      Serial.println("]}");
      last_button_state = state;
    }
  }
}
