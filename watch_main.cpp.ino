#include <Arduino.h>
#include <Wire.h>
#include <ChronosESP32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "graphic.h" // FIX: Now includes the singular file name: "graphic.h"

// Define OLED screen parameters (128x32 is standard for 0.96" OLED)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1 // Reset pin (or -1 if sharing Arduino reset pin)
#define OLED_SDA 10   // User's original SDA pin - CONFIRM YOUR PINS!
#define OLED_SCL 11   // User's original SCL pin - CONFIRM YOUR PINS!

// Initialize the Adafruit display object (using I2C address 0x3C, the most common)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Initialize Chronos Watch library
ChronosESP32 watch("OLED Watch");

// Pin Definitions
#define BUILTINLED 2
#define BUTTON 0 // ESP32 BOOT button (or other GPIO pin for the watch button)

// Global State Variables
static bool deviceConnected = false;
static int id = 0;
long timeout = 10000, timer = 0, scrTimer = 0;
bool rotate = false, flip = false, hr24 = true, notify = true, screenOff = false, scrOff = false, b1;
int scroll = 0, bat = 0, lines = 0, msglen = 0;
String msg;
String msg0, msg1, msg2, msg3, msg4, msg5;

// Function Prototypes
void showNotification();
void printLocalTime();
void copyMsg(String ms);
void button(bool b);
void printAligned(const String& text, int alignment, int y);

// Custom Font/Graphics Data (Defined in graphic.h)
extern const unsigned char bluetooth[];


/**
 * @brief Handles configuration updates from the phone app.
 */
void configCallback(Config config, uint32_t a, uint32_t b)
{
  switch (config)
  {
    case CF_USER:
      // Note: The original logic for flip/rotate modes is based on the
      // non-standard OLED_I2C library. We will use Adafruit's standard
      // setRotation for a close approximation.
      flip = ((b >> 24) & 0xFF) == 1;
      rotate = ((b >> 8) & 0xFF) == 1;
      timeout = ((b >> 16) & 0xFF) * 1000;
      break;
  }
}

/**
 * @brief Handles incoming notifications.
 */
void notificationCallback(Notification notification)
{
  Serial.println("Notification received:");
  Serial.print("From: ");
  Serial.println(notification.app);
  Serial.println(notification.message);

  timer = millis();
  // Limit message length to prevent buffer overflow or excessive scrolling
  msg = notification.message.substring(0, 126); 
  msglen = msg.length();
  
  // Calculate lines based on 21 chars/line (approx for SmallFont)
  lines = ceil(float(msglen) / 21);
  scroll = 0;
  scrOff = false; // Turn screen on for notification
}

/**
 * @brief Handles incoming phone calls.
 */
void ringerCallback(String caller, bool state)
{
  if (state)
  {
    Serial.print("Ringer: Incoming call from ");
    Serial.println(caller);

    timer = millis() + 15000; // Keep message up for 15 seconds
    msg = "Incoming call        " + caller;
    msglen = msg.length();
    lines = ceil(float(msglen) / 21);
    scroll = 0;
    scrOff = false;
  }
  else
  {
    Serial.println("Ringer dismissed");
    timer = millis() - timeout; // Dismiss immediately
    scrOff = true;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-S3 Watch");

  // Pin setup
  pinMode(BUILTINLED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  // Initialize I2C for the OLED
  // NOTE: On the ESP32, Wire.begin() with no arguments typically uses the default
  // pins (e.g., GPIO 21/22), but since you specified SDA/SCL, we use those.
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize display
  // Use 0x3C or 0x3D as the I2C address, depending on your module. 0x3C is very common.
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed if display fails
  }
  
  display.display(); // Show initial Adafruit logo
  delay(2000);
  display.clearDisplay();

  // Set up Chronos
  watch.setConfigurationCallback(configCallback);
  watch.setNotificationCallback(notificationCallback);
  watch.setRingerCallback(ringerCallback);
  watch.begin();

  // Initial settings
  watch.set24Hour(true);
  watch.setBattery(80);

  timer = millis() - timeout;
}

void loop() {
  // Update Chronos state
  watch.loop();

  // Handle button press
  button(digitalRead(BUTTON) == LOW);

  // --- Display Setup ---
  display.clearDisplay();

  // Set rotation based on config (0: normal, 1: 90 deg, 2: 180 deg, 3: 270 deg)
  // We approximate the original logic:
  // rotate=true -> 180 degrees (for HUD/upside-down viewing)
  if (rotate) {
    display.setRotation(2); // 180 degrees
  } else {
    display.setRotation(0); // 0 degrees (normal)
  }
  
  // Handle screen off/sleep mode
  if (scrOff && screenOff) {
    // Turn display off command
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  } else {
    // Turn display on command (default state)
    display.ssd1306_command(SSD1306_DISPLAYON);
  }

  // --- Logic ---
  digitalWrite(BUILTINLED, watch.isConnected());

  long cur = millis();
  if (cur < timer + timeout) {
    // Show Notification Mode
    scrTimer = cur;
    long rem = (timer + timeout - cur);
    if (lines > 3) {
      // Map remaining time to scroll position
      // Original map logic: scroll if longer than 3 lines
      if (rem < timeout - (timeout / 3) && rem > (timeout / 3)) {
        scroll = map(rem, timeout - (timeout / 3), (timeout / 3), 0, (lines - 3) * 11);
      }
    }
    showNotification();
  } else {
    // Display Time Mode
    if (!scrOff && screenOff && cur > scrTimer + timeout) {
      scrOff = true; // Turn screen off after timeout
    }
    
    // Only draw if screen is not off
    if (!(scrOff && screenOff)) {
      printLocalTime();
    }
  }

  // Final display update
  display.display();
}

/**
 * @brief Prints text aligned to LEFT (0), CENTER (1), or RIGHT (2) at a given Y coordinate.
 * Note: All coordinates in Adafruit are 0-based.
 */
void printAligned(const String& text, int alignment, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  // Use getTextBounds to calculate text width based on the current font/size settings
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int x;

  if (alignment == 0) { // LEFT
    x = 0;
  } else if (alignment == 1) { // CENTER
    x = (SCREEN_WIDTH - w) / 2;
  } else { // RIGHT (alignment == 2)
    x = SCREEN_WIDTH - w;
  }

  display.setCursor(x, y);
  display.print(text);
}

/**
 * @brief Draws the active notification message.
 */
void showNotification() {
  // Use default Adafruit font (size 1 for SmallFont replacement)
  display.setFont(NULL); 
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  copyMsg(msg);
  if (msglen == 0) {
    msg0 = "No messsage";
  }

  // Use 9 pixels per line height (y=0, 9, 18, 27...) to fit text on a 32-pixel screen
  int line_height = 9;

  // Print all lines, adjusted by scroll offset
  printAligned(msg0, 0, 0 - scroll);
  printAligned(msg1, 0, line_height - scroll);
  printAligned(msg2, 0, line_height * 2 - scroll);
  printAligned(msg3, 0, line_height * 3 - scroll);
  printAligned(msg4, 0, line_height * 4 - scroll);
  printAligned(msg5, 0, line_height * 5 - scroll);
}

/**
 * @brief Draws the current time and status indicators.
 */
void printLocalTime() {
  display.setTextColor(SSD1306_WHITE);

  // 1. Draw AM/PM indicator (Size 1 for SmallFont replacement)
  display.setFont(NULL);
  display.setTextSize(1);
  printAligned(watch.getAmPmC(true), 2, 10); // RIGHT aligned at Y=10

  // 2. Draw Time (Size 2 for MediumNumbers replacement)
  // Size 2 text is 16 pixels high. Placed near the top (Y=0)
  display.setTextSize(2);
  printAligned(watch.getHourZ() + watch.getTime(":%M"), 1, 0); // CENTER aligned

  // 3. Draw Date (Size 1 for SmallFont replacement)
  display.setTextSize(1);
  // Placed at Y=21
  printAligned(watch.getTime("%a %d %b"), 1, 21); // CENTER aligned

  // 4. Status Indicators (Bluetooth and Battery)
  if (watch.isConnected()) {
    // Bluetooth Icon (16x16)
    // The icon is placed at the far left, lower half of the screen.
    display.drawBitmap(0, 15, bluetooth, 16, 16, SSD1306_WHITE); // X=0, Y=15

    // Phone Battery Indicator
    int bat_x = 110;
    int bat_y = 23;
    int bat_w = 17;
    int bat_h = 7;
    int bat_cap_x = bat_x + bat_w;
    int bat_cap_w = 2;
    int level = watch.getPhoneBattery();
    
    // Draw battery body (rectangle)
    display.drawRect(bat_x, bat_y, bat_w, bat_h, SSD1306_WHITE);
    // Draw battery cap
    display.fillRect(bat_cap_x, bat_y + 1, bat_cap_w, bat_h - 2, SSD1306_WHITE);

    // Calculate fill width 
    int fill_width = map(level, 0, 100, 0, bat_w - 2); 
    
    // Draw battery fill. 
    if (level > 0) {
      display.fillRect(bat_x + 1, bat_y + 1, fill_width, bat_h - 2, SSD1306_WHITE);
    }
  }
}

/**
 * @brief Splits the notification message into lines (max 21 chars per line).
 */
void copyMsg(String ms) {
  // Use a constant line length of 21 characters for splitting
  const int LINE_LEN = 21;

  // Reset all message lines
  msg0 = msg1 = msg2 = msg3 = msg4 = msg5 = "";

  // Populate message lines based on the calculated 'lines' count
  if (lines >= 1) msg0 = ms.substring(0, min((int)ms.length(), LINE_LEN));
  if (lines >= 2) msg1 = ms.substring(LINE_LEN, min((int)ms.length(), LINE_LEN * 2));
  if (lines >= 3) msg2 = ms.substring(LINE_LEN * 2, min((int)ms.length(), LINE_LEN * 3));
  if (lines >= 4) msg3 = ms.substring(LINE_LEN * 3, min((int)ms.length(), LINE_LEN * 4));
  if (lines >= 5) msg4 = ms.substring(LINE_LEN * 4, min((int)ms.length(), LINE_LEN * 5));
  if (lines >= 6) msg5 = ms.substring(LINE_LEN * 5, min((int)ms.length(), LINE_LEN * 6));
}

/**
 * @brief Handles the physical button press for screen wake/notification dismissal.
 */
void button(bool b) {
  if (b) {
    if (!b1) {
      // button debounce
      long current = millis();
      
      if (scrOff) {
        // Screen is off, turn it on
        scrOff = false;
        scrTimer = current;
      } else {
        if (current < timer + timeout) {
          // Notification is active, dismiss it
          timer = current - timeout;
        } else {
          // Screen is on, but showing time. Refresh timer (effectively prevents screen-off)
          timer = current; 
        }
        scroll = 0;
        scrOff = false;
      }

      // end button debounce
      b1 = true;
    }
  } else {
    b1 = false;
  }
}