#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

// Choose I2C interface (we're using I2C, not SPI)
#define SSD1306_USE_I2C

// I2C handle from CubeMX
#define SSD1306_I2C_PORT hi2c1
#define SSD1306_I2C_ADDR (0x3C << 1)

// Display geometry
#define SSD1306_HEIGHT 64
#define SSD1306_WIDTH  128

// CRITICAL for SH1106: 1.3" OLEDs use SH1106 controller
// which has 132 columns of memory but only 128 visible.
// The visible window starts 2 columns in.
#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_UPPER 0

// Mirror options (leave off)
// #define SSD1306_MIRROR_VERT
// #define SSD1306_MIRROR_HORIZ
// #define SSD1306_INVERSE_COLOR

// Include all fonts (we'll use Font_7x10 mostly)
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18
#define SSD1306_INCLUDE_FONT_16x26

#endif /* __SSD1306_CONF_H__ */
