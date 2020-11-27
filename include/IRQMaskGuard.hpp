#ifndef _IRQMASKGUARD

#define _IRQMASKGUARD

#include <xinu.h>

class IRQMaskGuard
{
    intmask mask;

public:
    IRQMaskGuard() : mask(disable()) {}
    IRQMaskGuard(const IRQMaskGuard &) = delete;
    IRQMaskGuard(IRQMaskGuard &&) = delete;

    ~IRQMaskGuard() { restore(mask); }
};

#endif