
#ifndef GFX_H
#define GFX_H

#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <ADS7846.h>

#define min(a,b) {a < b ? a : b}


extern Adafruit_ILI9341 tft;


class LCD_HAL_Interface : public LCD_HAL
{
  Adafruit_ILI9341 &tft;
  
public:
  LCD_HAL_Interface(Adafruit_ILI9341 &);
  void fillScreen(int) const;
  void drawText( int, int, const char*, int, int, int ) const;
  int  getWidth() const;
  int  getHeight() const;
  void drawCircle( int, int, int, int ) const;
  void fillCircle( int, int, int, int ) const;
};


unsigned long testFillScreen();


#endif // GFX_H

