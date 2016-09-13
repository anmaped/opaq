
// Opaq_command allows us to execute commands from diferent modules/plugins, and
// recovery actions can be taked outside of the opaq controller by using the terminal.


#ifndef OPAQCOMMAND_H
#define OPAQCOMMAND_H

#include <Arduino.h>
#include <functional>
#include <atomic>

#include <LinkedList.h>

struct oq_cmd
{
	//oq_cmd() : args (LinkedList<String>()) {};
	//oq_cmd(const oq_cmd &c) { *this = c; };
	std::function<void (LinkedList<String> & args)> exec;
	LinkedList<String> args;

	//oq_cmd& operator=(const oq_cmd& a) { exec = a.exec; args = a.args; };
};

class Opaq_command
{
private:
	static std::atomic<bool> ll;
	LinkedList<oq_cmd> queue;

public:
	Opaq_command() : queue (LinkedList<oq_cmd>()) { };

	void begin();
	void end();

	void send(oq_cmd& cmd);
	void exec();
	void handler();
	void terminal();
	void terminalHandler();
	
};

extern Opaq_command command;

#endif // OPAQCOMMAND_H
