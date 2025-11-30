Overview

This project demonstrates how to use an ESP32 microcontroller with a small OLED display to receive and display Android notifications via Bluetooth Low Energy (BLE). When paired with the companion Android app Chronos, the ESP32 can also automatically synchronize the current time and function as a standalone wearable-style digital clock.

The project showcases practical IoT concepts including BLE communication, embedded display interfaces, and microcontroller-to-mobile integration.

Features

Receive Android notifications via BLE

Clock display with automatic time synchronization

Supports 12-hour and 24-hour time formats

OLED display rotation and flip options

Screen timeout and notification timeout controls

Works with the official Chronos Android app

Project Functionality

The ESP32 acts as a Bluetooth LE peripheral, waiting for a connection from the Chronos Android app. Once connected:

Notifications received by the smartphone are forwarded to the ESP32.

The device displays notification text on the OLED screen.

The app automatically sends the correct time to the ESP32.

The device displays a digital clock when idle.

This makes the project useful as a simple wearable device, a BLE debugging tool, or a smart desk clock.

Required Hardware

ESP32 development board

128x64 or 128x32 I2C OLED display (SSD1306 recommended)

USB cable for programming

Optional: enclosure or watch straps

Required Software
Arduino IDE

Ensure you have the ESP32 boards installed under:

Boards Manager → ESP32 by Espressif Systems
Libraries

Install these through the Arduino Library Manager or manually:

ChronosESP32

OLED_I2C (modified to include flipMode() behavior)

Place the libraries into your Arduino libraries/ folder if installing manually.

File Structure
ESP32_OLED_Watch/
│
├── watch_main.cpp.ino
├── graphic.h
├── README.md
└── libraries/
    ├── ChronosESP32
    └── OLED_I2C

watch_main.cpp.ino contains the main program logic.

graphic.h stores icons/bitmaps used in the OLED display.

Android App Setup

Use the Chronos Android app:

Open the Watch tab.

Tap Watches.

Select Pair new watch.

Choose your ESP32 from the BLE device list.

Configure display options:

Flip display: Toggle Units (Metric/Imperial)

Rotate display: Toggle Temperature (°C/°F)

Videos

The original repository includes demonstration videos such as:

Chronos ESP32 OLED notifications

ESP32 OLED notifications

How to connect and pair

License

This project is covered by the MIT License:

You may use, modify, distribute, and sell the software.

You must include the MIT license text in any redistributed code.

Copyright (c) 2021 Felix Biego

Credits

Project and original source code by Felix Biego. Modified or repackaged versions should retain attribution per MIT License.
