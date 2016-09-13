
// opaq command
#include "Opaq_command.h"

std::atomic<bool> Opaq_command::ll;

Opaq_command command = Opaq_command();

void Opaq_command::begin()
{
	
}

void Opaq_command::end()
{
	
}

void Opaq_command::send(oq_cmd& cmd)
{
	// atomicity to aovid concurrent adds
	bool b = false;
	if( std::atomic_compare_exchange_strong<bool>(&ll, &b, true) )
	{
		// enqueue command
		queue.add(cmd);

		ll = false;
	}
}

void Opaq_command::exec()
{
	if(queue.size() != 0)
	{
		oq_cmd c;
		c = queue.pop();
		c.exec(c.args);
	}

}

void Opaq_command::handler()
{
	// for each queued command do
	for(unsigned short int i=0; i < queue.size(); i++)
	{
		exec();
	}

}

void Opaq_command::terminal()
{
	// show available commands

	// activate handler to parse commands

}

void Opaq_command::terminalHandler()
{

}
