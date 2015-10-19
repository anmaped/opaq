/*
 *  Opaq is an Open AQuarium Controller firmware. It has been developed for
 *  supporting several aquarium devices such as ligh dimmers, power management
 *  outlets, water sensors, and peristaltic pumps. The main purpose is to
 *  control fresh and salt water aquariums.
 *
 *    Copyright (c) 2015 Andre Pedro. All rights reserved.
 *
 *  This file is part of opaq firmware for aquarium controllers.
 *
 *  opaq firmware is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  opaq firmware is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with opaq firmware.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
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
