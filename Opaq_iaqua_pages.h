
#ifndef OPAQ_IAQUA_PAGES_H
#define OPAQ_IAQUA_PAGES_H

#include "Opaq_iaqua.h"

class Opaq_iaqua_page_welcome : Opaq_iaqua {
public:
  void draw();
  void setExecutionBar(byte range);
  void msg(const char *msg);
  void clear();
};

#endif // OPAQ_IAQUA_PAGES_H
