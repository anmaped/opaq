
#include "Rf433ook.h"

#define BUFFER_SIZE 32

static byte buffer_idx = 0;
static byte consumed_idx = 0;
static byte buffer[BUFFER_SIZE] = {0};


void rf433mhz(uint32_t code, uint8_t state)
{
  bool vector[36] = { 0 };
  // vector is organized as follows :
  //              |---------------------------------------ID------------------------------------|--GROUPFLAG--|--ON/OFF--|---GROUPID---|-DIMMER-|
  // GLOBAL OFF -- unbinds everything
  // encoding   : 01 01 10 10 10 10 01 10 10 10 10 10 10 01 01 01 10 01 10 10 10 10 10 10 10 01 |       10         01    | 01 01 01 01 | UNDEF
  // bit stream : 0  0  1  1  1  1  0  1  1  1  1  1  1  0  0  0  1  0  1  1  1  1  1  1  1  0  |       1          0     |  0  0  0  0 | UNDEF
  // GLOBAL ON -- do not bind (binds are only allowed when groupflag is zero)
  // bit stream : 0  0  1  1  1  1  0  1  1  1  1  1  1  0  0  0  1  0  1  1  1  1  1  1  1  0  |       1          1     |  0  0  0  0 | UNDEF

  int j = 0;
  while ( j < 26 && code )
  {
    vector[j++] = code & 1;
    code >>= 1;
  }


  if ( state == RF433_OFF )
  {
    vector[26] = 0;
    vector[27] = 0;
  }
  else if ( state == RF433_UNBINDING )
  {
    vector[26] = 1;
    vector[27] = 0;
  }
  else if ( state == RF433_BINDING )
  {
    // set group flag to zero for allowing bind operations
    vector[26] = 0;
    vector[27] = 1;
  }
  else
  {
    vector[26] = 1;
    vector[27] = 1;
  }

  /*for ( int i = 0; i < 36; i++ )
  {
    DEBUGV ( "%d,", vector[i] );
  }
  DEBUGV ( "\n" );*/

  for ( int i = 0; i < 6; i++ )
  {
    Rf433_transmitter.sendMessage ( vector );
    delay ( 50 );
  }
  
}

void rf433()
{
  uint32_t code = 0;
  uint8_t state = 0;

  // let test if there is messges to send ( message can be incomplete)
  while( buffer[consumed_idx] != 0 && (buffer[buffer[consumed_idx] + ((consumed_idx != 0)? 1:0 ) + 1] != 0) )
  {
    code |= buffer[consumed_idx + 1];
    code |= buffer[consumed_idx + 2] << 8;
    code |= buffer[consumed_idx + 3] << 16;
    code |= buffer[consumed_idx + 4] << 24;
    state = buffer[consumed_idx + 5];
    
    // one message to consume
    rf433mhz( code, state );
    
    consumed_idx += buffer[consumed_idx] +  ((consumed_idx != 0)? 1:0 )  + 1;
  }
  
}

// reponse for the interrupt routine
inline void enqueue_stream(byte data, byte count)
{
  if (count == 0)
  {
    memset(buffer, 0, BUFFER_SIZE);
    buffer_idx = 0;
    consumed_idx = 0;
  }
  
  if ( buffer_idx < BUFFER_SIZE )
  {
    buffer[buffer_idx++] = data;
  }
  
}

