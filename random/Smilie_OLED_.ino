#include <Wire.h>
#include <U8g2lib.h>

// Use default hardware I2C pins for ESP32 (SDA=21, SCL=22)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0, /* rotation=*/ 
  U8X8_PIN_NONE /* reset=*/
);

void setup() {
  // Initialize the display
  u8g2.begin();

  // --- Draw the Smiley Face ---

  // 1. Select the open-iconic font set, size 8x. These are large icons.
  u8g2.setFont(u8g2_font_open_iconic_all_8x_t);
  
  // 2. Clear the internal display buffer
  u8g2.clearBuffer();
  
  // 3. Draw the smiley character. The number 'G' (ASCII 71) corresponds to the smiley face in this font.
  //    Position it at (32, 64) to center it on the 128x64 display.
  u8g2.drawCircle(64, 32, 30); 
  u8g2.drawCircle(50, 25, 5); 
  u8g2.drawCircle(78, 25, 5); 
  u8g2.drawArc(64,32,20,0,5);
  u8g2.drawArc(64,32,20,140,240);  
  // 4. Send the buffer to the display to make it visible
    // u8g2.drawStr(32,32,":)");
  u8g2.sendBuffer();
}

void loop() {
  // Nothing to do here, the image is static.
}
