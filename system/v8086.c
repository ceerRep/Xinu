#include <xinu.h>

extern char code16_source_start[0];
extern char code16_source_end[0];
extern char CODE16[0];
extern char CODE16END[0];
extern char CODE16STACKTOP[0];

void v8086_init()
{
    if (code16_source_end - code16_source_start > CODE16END - CODE16)
        panic("16bit code too large");

    memcpy(CODE16, code16_source_start, code16_source_end - code16_source_start);
}

void v8086_call(void *func,
                uint16 ax,
                uint16 bx,
                uint16 cx,
                uint16 dx,
                uint16 si,
                uint16 di,
                uint16 ds,
                uint16 es)
{
    memset(&v8086_tss, 0, sizeof v8086_tss);
    v8086_tss.io[sizeof(v8086_tss.io) - 1] = 0xFF;
    v8086_tss.tss.link = 0;
    asm("movl %%cr3, %%eax\n\t"
        : "=a"(v8086_tss.tss.cr3)
        :
        :);

    // disable interrupt, enable v8086, set IOPL=3
    asm("pushfl\n\t"
        "pop %%eax\n\t"
        "andl $0xfffffdff, %%eax\n\t"
        "orl  $0x23000, %%eax"
        : "=a"(v8086_tss.tss.eflags)
        :
        :);

    v8086_tss.tss.cs = 0;
    v8086_tss.tss.eip = (uintptr)((char *)func - code16_source_start + CODE16);

    v8086_tss.tss.ss = 0;
    v8086_tss.tss.esp = v8086_tss.tss.esp0 = (uintptr)CODE16STACKTOP;

    v8086_tss.tss.ds = ds;
    v8086_tss.tss.es = es;

    v8086_tss.tss.eax = ax;
    v8086_tss.tss.ebx = bx;
    v8086_tss.tss.ecx = cx;
    v8086_tss.tss.edx = dx;
    v8086_tss.tss.esi = si;
    v8086_tss.tss.edi = di;

    v8086_tss.tss.link = 0;
    v8086_tss.tss.ldtr = 0;
    v8086_tss.tss.iopb_offset = sizeof(tss_t);

    for (int i = 0; i < 32; i++)
        v8086_tss.tss.redirect[i] = 0xFF;

    asm("lcall $0x30, $0\n\t");
}
