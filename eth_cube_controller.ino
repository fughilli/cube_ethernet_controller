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

const CH9121Config config = {
    .gateway = {192, 168, 0, 1},
    .subnet_mask = {255, 255, 255, 0},
    .local_ip = {192, 168, 0, 51},
    .target_ip = {192, 168, 0, 1},
    .local_port = 5000,
    .target_port = 5000,
    .baud_rate = 921600,
    .mode = 0x02,
};

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

// Create commands
LedCommand led_command(leds, NUM_PIXELS);
ConfigCommand config_command(NUM_PIXELS);
ReconfCommand reconf_command(ch9121);
LcdCommand lcd_command(lcd, LCD_WIDTH, LCD_HEIGHT);
Command* commands[] = {&led_command, &config_command, &reconf_command, &lcd_command};
CommandProcessor command_processor(commands);

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

  Serial.println("Starting CH9121 config...");
  ch9121.Begin();
  ch9121.Configure();
  Serial.println("Finished CH9121 config");
  
  // Clear LCD and show ready message
  lcd.clear();
  lcd.print("Ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial2.available()) {
    char value = Serial2.read();
    command_processor.process_char(value);
  }
}
