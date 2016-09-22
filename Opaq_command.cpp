
// opaq command
#include <cstdint>
#include <Fs.h>

#include "Opaq_command.h"
#include "Opaq_storage.h"
#include "Opaq_c1.h"
#include "slre.h"

#include "zmodem_config.h"
#include "zmodem.h"
#include "zmodem_zm.h"

std::atomic<bool> Opaq_command::ll;

Opaq_command command = Opaq_command();


namespace fnv1a_64
{
 
typedef std::uint64_t hash_t;
 
constexpr hash_t prime = 0x100000001B3ull;
constexpr hash_t basis = 0xCBF29CE484222325ull;
 
constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)
{
	return *str ? hash_compile_time(str+1, (*str ^ last_value) * prime) : last_value;
}
 
hash_t hash(char const* str)
{
	hash_t ret{basis};
 
	while(*str){
		ret ^= *str;
		ret *= prime;
		str++;
	}
 
	return ret;
}
 
}
 
constexpr unsigned long long operator "" _hash(char const* p, size_t)
{
	return fnv1a_64::hash_compile_time(p);
}


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

	terminal();

}

void Opaq_command::terminal()
{
	struct slre_cap caps[5];

	static String line = "";
    String lre = String(F("^([a-z0-9]+)(\\s[a-zA-Z0-9./-]*)*"));

    auto read_line = [](String &inData)
    {
    	// define a timeout
    	while (Serial.available() > 0)
    	{
    		char recieved = Serial.read();
    		//echo
    		Serial.write(recieved);

    		if(recieved != '\b')
    		{
    			inData += recieved;
    		}
    		else
    		{
    			inData = inData.substring(0, inData.length() - 1);
    		}

    		if (recieved == '\n' || recieved == '\r')
    		{
    			Serial.write('\n');
    			return true && (inData.length() > 0);
    		}

    		optimistic_yield(10000);

    	}

	    return false;
    };

    auto alert = [read_line]()
    {
    	String in = "";
    	Serial.println("Are you sure? ([Y]es/No):");

    	while(!read_line(in)) { optimistic_yield(10000); }

    	if(in.substring(0,1) == "y")
    	{
    		return true;
    	}
    	else
    	{
    		return false;
    	}
    };

    auto list_files = []() {
    	Dir d = SPIFFS.openDir("/");
		// list directory
    	Serial.println(F("LIST DIR: "));
    	while ( d.next() )
    	{
    		String filename = d.fileName();
    		Serial.printf("%s %d  ", filename.c_str(), d.fileSize() );
    		Serial.println("");
    	}
    };

    auto split = [](String& input, String * out)
    {
    	int counter = 0;
    	int lastIndex = 0;

    	for (int i = 0; i < input.length(); i++) {
	        // Loop through each character and check if it's a space
	        if (input.substring(i, i+1) == " ") {
	        	// Grab the piece from the last index up to the current position and store it
	        	out[counter] = input.substring(lastIndex, i);
	        	// Update the last position and add 1, so it starts from the next character
	        	lastIndex = i + 1;
	        	// Increase the position in the array that we store into
	        	counter++;
	        }

	        // If we're at the end of the string (no more commas to stop us)
	        if (i == input.length() - 1) {
	          // Grab the last part of the string from the lastIndex to the end
	          out[counter] = input.substring(lastIndex, i);
			}
		}
    };

    // get line otherwise exit
    if(!read_line(line))
    	return;

	// lets parse the line
	if (slre_match(lre.c_str(), line.c_str(), line.length(), caps, 5, 0) > 0)
	{

		//Serial.print(F("opaq>"));
		// echo
		//Serial.println(line);

		String command = "";
        command += caps[0].ptr;
        command.setCharAt(caps[0].len, '\0');
        DEBUG_MSG_COMMAND(FF("command=%s\r\n"), command.c_str());

        String args = "";
        String arg[2];
        arg[0] = "";
        arg[1] = "";

		switch (fnv1a_64::hash(command.c_str()))
		{
			case "format"_hash 	:
									if ( alert() )
									{
										if(SPIFFS.format())
										{
											Serial.println(F("Format successful"));
										}
										else
										{
											Serial.println(F("Format unsuccessful"));
										}
									}
									break;
			case "rm"_hash		:   
									args = caps[1].ptr;
									Serial.println(caps[1].len);
									args = args.substring(1);
									args.setCharAt(caps[1].len - caps[0].len - 1, '\0');
									Serial.println(args.c_str());

							        if(SPIFFS.exists(args.c_str()))
							        {
							        	Serial.println("Removed.");
							        	SPIFFS.remove(args.c_str());
							        }
							        else
							        {
							        	Serial.println("Not found!");
							        }
									break;

			case "mv"_hash		:
									args = caps[1].ptr;
									Serial.println(caps[1].len);
									args = args.substring(1);
									args.setCharAt(caps[1].len - caps[0].len, '\0');
									Serial.println(args.c_str());

									//args.split(' ');
									split(args, arg);

									Serial.println(arg[0]);
									Serial.println(arg[1]);

									SPIFFS.rename(arg[0].c_str(), arg[1].c_str());
									Serial.println("File renamed");
									break;

			case "update"_hash 	:	/*storage.fwupdate("noname.bin", "");*/
									break;

			case "tar"_hash     :	
									args = caps[1].ptr;
									Serial.println(caps[1].len);
									args = args.substring(1);
									args.setCharAt(caps[1].len - caps[0].len, '\0');
									Serial.println(args.c_str());

									//args.split(' ');
									split(args, arg);

									Serial.println(arg[0]);
									Serial.println(arg[1]);
									// check inputs [todo]
									if(SPIFFS.exists(arg[0]))
									{
										storage.tarextract(arg[0].c_str(), arg[1].c_str());
										Serial.println("tar successful");
									}
									else
									{
										Serial.println("tar unsuccessful");
									}

									break;

			case "ls"_hash      :	
									list_files();
									break;
			case "wifi"_hash    :
    								WiFi.printDiag(Serial);
    								break;

    		case "nrf24"_hash   :
    								communicate.nrf24.printstate();
    								break;

    		case "mount"_hash	:
    								if(!SPIFFS.begin())
    									Serial.println(F("Issues"));
    								else
    									Serial.println(F("Mounted"));
    								break;

			case "rz"_hash      :	
									//SPIFFS.garbage();
									//wdt_enable(60000);
									//ets_wdt_disable();
									Serial.setDebugOutput ( false );

									if ( wcreceive(0, 0) )
									{
								    	Serial.println(F("zmodem transfer failed"));
								    }
								    else
								    {
								    	Serial.println(F("zmodem transfer successful"));
								    }

								    Serial.setDebugOutput ( true );
								    break;

			case "reboot"_hash	:	
									ESP.restart();
									break;

			case "fscls"_hash :	
									for(int i=0; i<100; i++)
									SPIFFS.garbage();
									Serial.println("Filesystem cleanup.");
									break;

			case "state"_hash	:	
									Serial.print("Heap: ");
									Serial.println(ESP.getFreeHeap());
									break;

			case "help"_hash 	:
									// show available commands
									Serial.println(F("Available commands: \r\nformat \r\nupdate \r\ntar \r\nls \r\nrm \r\nwifi \r\nnrf24 \r\nrz \r\nfscls \r\nhelp"));
									break;
			default 			:
									Serial.println(F("Unknown command."));
									break;
		}

		Serial.print(F("opaq>"));

	}
	else
	{
		// echo
		Serial.println(line);
		Serial.println(F("Unknown command."));
		Serial.print(F("opaq>"));
	}

	// reset string
	line = "";

	// activate handler to parse commands

}

