

#ifndef CAS_H
#define CAS_H

#include <Arduino.h>

extern "C" bool __atomic_compare_exchange_1(volatile bool*, bool*, bool, bool, int, int);

bool cas(volatile uint8_t *mem, uint8_t expected, uint8_t desired);

#endif // CAS_H

