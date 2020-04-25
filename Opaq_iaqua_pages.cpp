

#include "Opaq_iaqua_pages.h"

#include "gfx.h"

void Opaq_iaqua_page_welcome::draw() {
  tft.fillScreen(ILI9341_BLACK);
  myFiles.load(26, 110, 188, 72, "opaq.raw");

  tft.drawRoundRect(20, 250, 200, 10, 5, RGB(55, 55, 55));
}

void Opaq_iaqua_page_welcome::setExecutionBar(byte range) {
  if (range > 5 && range <= 100)
    tft.fillRoundRect(20, 250, (2 * range), 10, 5, RGB(80, 80, 80));
}

void Opaq_iaqua_page_welcome::msg(const char *msg) {
  tft.fillRoundRect(60, 260, 240, 20, 0, RGB(0, 0, 0));
  myGLCD.setFont(NULL); // TODO
  myGLCD.setColor(55, 55, 55);
  myGLCD.print(msg, 60, 270);
}

void Opaq_iaqua_page_welcome::clear() {
  tft.fillScreen(ILI9341_BLACK);
}
