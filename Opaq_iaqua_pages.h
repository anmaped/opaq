
#include "Opaq_iaqua.h"

class Opaq_iaqua_page_welcome : Opaq_iaqua {
public:
  void draw();
  void setExecutionBar(byte range);
  void msg(const char *msg);
};
