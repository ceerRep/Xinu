#ifndef _TSS_H

#define _TSS_H

#include "xinu.h"

typedef struct tss
{
    uint32 link;
    uint32 esp0;
    uint32 ss0;
    uint32 esp1;
    uint32 ss1;
    uint32 esp2;
    uint32 ss2;
    uint32 cr3;
    uint32 eip;
    uint32 eflags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint32 es;
    uint32 cs;
    uint32 ss;
    uint32 ds;
    uint32 fs;
    uint32 gs;
    uint32 ldtr;
    uint16 padding;
    uint16 iopb_offset;
    char redirect[32];
} tss_t;

typedef struct full_tss
{
    tss_t tss;
    char io[65536 / 8 + 1];
} full_tss_t;

void tssinit();
extern tss_t kernel_tss, page_fault_tss, gp_tss;

extern full_tss_t v8086_tss;

#endif
