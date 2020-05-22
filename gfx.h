
#ifndef GFX_H
#define GFX_H

#include "opaq.h"
#include "src/ADS7846/ADS7846.h"
#include <Adafruit_ILI9341.h>
#include <Arduino.h>

#define min(a, b)                                                              \
  { a < b ? a : b }

class LCD_HAL_Interface : public LCD_HAL {
  Adafruit_ILI9341 &tft;

public:
  LCD_HAL_Interface(Adafruit_ILI9341 &);
  void fillScreen(int) const;
  void drawText(int, int, const char *, int, int, int) const;
  int getWidth() const;
  int getHeight() const;
  void drawCircle(int, int, int, int) const;
  void fillCircle(int, int, int, int) const;
};

extern Adafruit_ILI9341 tft;
extern LCD_HAL_Interface tft_interface;

unsigned long testFillScreen();
unsigned long testText();
unsigned long testFilledRects(uint16_t color1, uint16_t color2);

#endif // GFX_H
