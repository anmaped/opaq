#ifndef RF433OOK_H
#define RF433OOK_H

class Rf433ook
{
  int encoding;
  int pin;
  
public:
  enum Device { CHANON_DIO_DEVICE };
  
  Rf433ook();
  
  void set_pin(int pin);
  void set_encoding(int device);
  void sendOOK(bool b);
  void sendBit(bool data_bit);
  void sendMessage(bool bit_vector[]);
  
};

extern Rf433ook Rf433_transmitter;

#endif // RF433OOK_H
