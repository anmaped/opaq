
#include "cas.h"

bool __atomic_compare_exchange_1(volatile bool * mem, bool * expected, bool desired, bool weak, int success, int failure)
{
	bool status = true;
	
	noInterrupts();

	if (*mem == *expected)
		*mem = desired;
	else
		status = false;

	interrupts();

	return status;
}


static inline unsigned long arch_local_irq_save(void)
{
        unsigned long flags;
        asm volatile("rsil %0, 1"
                     : "=a" (flags) :: "memory");
        return flags;
}

static inline void arch_local_irq_restore(unsigned long flags)
{
        asm volatile("wsr %0, ps; rsync"
                     :: "a" (flags) : "memory");
}



bool cas(volatile uint8_t *mem, uint8_t expected, uint8_t desired)
{
  bool status = true;
  noInterrupts();
  if (*mem == expected)
    *mem = desired;
  else
    status = false;
  interrupts();

  return status;
}


