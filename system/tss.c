#include <xinu.h>

_Static_assert(sizeof(tss_t) == 104, "Sizeof tss_t != 104");

tss_t kernel_tss, page_fault_tss;

static char page_fault_stack[4096];

void PageFaultHandler();

void tssinit()
{
    // 4
    memset(&kernel_tss, sizeof(kernel_tss), 0);
    kernel_tss.link = 0;
    kernel_tss.iopb_offset = 104;

    // 5
    memset(&page_fault_tss, sizeof(page_fault_tss), 0);
    page_fault_tss.link = 0;
    page_fault_tss.eip = (uintptr)&PageFaultHandler;

    asm("movl %%cr3, %%eax\n\t"
        : "=a"(page_fault_tss.cr3)
        :
        :);

    // disable interrupt
    asm("pushfl\n\t"
        "pop %%eax\n\t"
        "andl $0xfffffdff, %%eax"
        : "=a"(page_fault_tss.eflags)
        :
        :);

    page_fault_tss.esp = page_fault_tss.esp0 = (uintptr)page_fault_stack + 4080;

    page_fault_tss.ds =
        page_fault_tss.es =
            page_fault_tss.fs =
                page_fault_tss.gs = 0x10;
    page_fault_tss.cs = 0x8;
    page_fault_tss.ss = page_fault_tss.ss0 = 0x18;

    page_fault_tss.ldtr = 0;
    page_fault_tss.iopb_offset = 104;

    asm("mov $0x20, %ax\n\t"
        "ltr %ax\n\t");

    int32 set_task_evec(uint32 xnum, uint32 handler);

    // Page fault
    set_task_evec(14, 0x28);
}