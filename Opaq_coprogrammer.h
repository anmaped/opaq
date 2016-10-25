
#include <ESP8266AVRISP.h>

#include "opaq.h"


#ifdef DEBUG_ESP_OPAQCOPROG
#define DEBUG_MSG_OPAQCOPROG(...) DEBUG_ESP_PORT.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG_OPAQCOPROG(...) 
#endif


class Opaq_coprogrammer : public ESP8266AVRISP
{
public:
	Opaq_coprogrammer(int port, byte pin) : ESP8266AVRISP(port, pin) {};
	void program(const char * filename);
	void set_param_atmega328p();
	void write_flash_page(int addrpage, byte * buff, int length);
	void read_signature(byte& high, byte& middle, byte& low);
	byte universal(byte a, byte b, byte c, byte d);
	
};
