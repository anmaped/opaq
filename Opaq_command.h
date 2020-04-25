
/*
 * Opaq_command allows us to execute commands from diferent modules/plugins, and
 * recovery actions that can be taked outside of the opaq controller by using
 * the terminal.
 */

#ifndef OPAQCOMMAND_H
#define OPAQCOMMAND_H

#include <Arduino.h>
#include <LinkedList.h>
#include <Scheduler.h>
#include <Scheduler/Semaphore.h>
#include <atomic>
#include <functional>

#include "opaq.h"

#ifdef DEBUG_ESP_OPAQCOMMAND
#define DEBUG_MSG_COMMAND(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define DEBUG_MSG_COMMAND(...)
#endif

struct oq_cmd {
  // oq_cmd() : args (LinkedList<String>()) {};
  // oq_cmd(const oq_cmd &c) { *this = c; };
  std::function<void(LinkedList<String> &args)> exec;
  LinkedList<String> args;

  // oq_cmd& operator=(const oq_cmd& a) { exec = a.exec; args = a.args; };
};

class Opaq_command {
private:
  static std::atomic<bool> ll;
  LinkedList<oq_cmd> queue;
  Semaphore cmd_lock;

public:
  Opaq_command() : queue(LinkedList<oq_cmd>()), cmd_lock(Semaphore()){};

  void begin();
  void end();

  void send(oq_cmd &cmd);
  void exec();
  void handler();
  void terminal();

  void lock() { cmd_lock.wait(); }
  void unlock() { cmd_lock.signal(); }
};

extern Opaq_command command;

#endif // OPAQCOMMAND_H
