#ifndef _V8086_H

#define _V8086_H

#include <xinu.h>

extern char code16_source_int10[0];

void v8086_init();

void v8086_call(void *func,
                uint16 ax,
                uint16 bx,
                uint16 cx,
                uint16 dx,
                uint16 si,
                uint16 di,
                uint16 ds,
                uint16 es);

#endif
