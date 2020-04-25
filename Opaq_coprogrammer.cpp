
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <SPI.h>

#include "Opaq_coprogrammer.h"

void Opaq_coprogrammer::program(const char *filename) {
  byte buf[200];
  int npages = 1;

  DEBUG_MSG_OPAQCOPROG("PROGRAM %s\r\n", filename);

  File tmp = SPIFFS.open(filename, "r");

  set_param_atmega328p();

  start_pmode();

  byte h, m, l;
  read_signature(h, m, l);

  DEBUG_MSG_OPAQCOPROG("%02x %02x %02x\r\n", h, m, l);

  // format before programming it
  byte erase = universal(0xac, 0x80, 0x00, 0x00);

  DEBUG_MSG_OPAQCOPROG("Erase result= %02x\r\n", erase);

  byte lfuse = 0, hfuse = 0, efuse = 0;
  while (lfuse != 0xff) {
    lfuse = universal(0x50, 0x00, 0x00, 0x00);
    DEBUG_MSG_OPAQCOPROG("lfuse= %02x\r\n", lfuse);
    delay(100);
  }

  while (hfuse != 0xda) {
    hfuse = universal(0x58, 0x08, 0x00, 0x00);
    DEBUG_MSG_OPAQCOPROG("hfuse= %02x\r\n", hfuse);
    delay(100);
  }

  while (efuse != 0xfd) {
    efuse = universal(0x50, 0x08, 0x00, 0x00);
    DEBUG_MSG_OPAQCOPROG("efuse= %02x\r\n", efuse);
    delay(100);
  }

  end_pmode();

  delay(1000);

  start_pmode();

  DEBUG_MSG_OPAQCOPROG("%d\r\n", tmp.size() / param.pagesize);
  for (int i = 0; i < tmp.size() / 2; i += param.pagesize / 2) {
    DEBUG_MSG_OPAQCOPROG("Writing %04x...", i);
    byte lread = tmp.read(buf, param.pagesize);
    DEBUG_MSG_OPAQCOPROG("read %04x\r\n", lread);
    write_flash_page(i, buf, lread);
  }

  end_pmode();

  tmp.close();
}

void Opaq_coprogrammer::set_param_atmega328p() {
  param.devicecode = 0x86;
  param.revision = 0x00;
  param.progtype = 0x00;
  param.parmode = 0x01;
  param.polling = 0x01;
  param.selftimed = 0x01;
  param.lockbytes = 0x01;
  param.fusebytes = 0x03;
  param.flashpoll = 0xff;
  param.eeprompoll = 0xffff;

  param.pagesize = 0x80;
  param.eepromsize = 0x400;
  param.flashsize = 0x8000;
}

void Opaq_coprogrammer::write_flash_page(int addrpage, byte *buff, int length) {
  int x = 0;
  int page = addr_page(addrpage);

  while (x < length) {
    yield();
    if (page != addr_page(addrpage)) {
      commit(page);
      page = addr_page(addrpage);
    }
    flash(LOW, addrpage, buff[x++]);
    flash(HIGH, addrpage, buff[x++]);
    addrpage++;
  }

  commit(page);
}

void Opaq_coprogrammer::read_signature(byte &high, byte &middle, byte &low) {
  high = spi_transaction(0x30, 0x00, 0x00, 0x00);
  middle = spi_transaction(0x30, 0x00, 0x01, 0x00);
  low = spi_transaction(0x30, 0x00, 0x02, 0x00);
}

byte Opaq_coprogrammer::universal(byte a, byte b, byte c, byte d) {
  return spi_transaction(a, b, c, d);
}
