
#ifdef ARDUINO
#ifndef ESP8266

File * _fopen(const char * filename, const char * mode)
{
// if (!fout.open(Pathname, O_WRITE | O_CREAT | O_AT_END))
}

void _fclose(File * fl)
{
    fl.flush();
    fl.sync();
    fl.close();
}

void _putc(const char p, File * fl)
{
    fl.write(p);
}

#else

#include <FS.h>

File * _fopen(const char * filename, const char * mode)
{
    //Serial.println("Open!");
    File * x = new File();
    *x = SPIFFS.open(String("/") + filename, "w");

    if(!(*x))
    {
        return NULL;
    }

    return x;
}

void _fclose(File * fl)
{
    if(fl != NULL)
    {
        fl->close();

        delete fl;
    }

    //Serial.println("Close!");
}

void _putc(const char p, File * fl)
{
    fl->write(p);
    fl->flush();

    optimistic_yield(10000);
}
#endif
#endif


