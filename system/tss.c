#include <xinu.h>

extern struct sd gdt[0];

_Static_assert(sizeof(tss_t) == 104 + 32, "Sizeof tss_t != 104 + 32");

tss_t kernel_tss, page_fault_tss, gp_tss;

full_tss_t v8086_tss;

tss_t *tss_array[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    &kernel_tss,
    &page_fault_tss,
    &v8086_tss.tss,
    &gp_tss};

static char page_fault_stack[4096];

void PageFaultHandler();

void GeneralProtectionHandler()
{
    asm("1:\n\t"
        "call GeneralProtectionHandler_\n\t"
        "pop %eax\n\t" // unused error code
        "iret\n\t"
        "jmp 1b\n\t");
}

void GeneralProtectionHandler_(int error_code)
{
    uint32 link = gp_tss.link;
    tss_t *prev = tss_array[gp_tss.link / 8];
    uint32 cs = prev->cs;
    uint32 eip = prev->eip;
    if (prev == &v8086_tss.tss && prev->eflags & 0x20000) // v8086
    {
        uint8 *addr = (uint8 *)(cs * 16 + eip);
        uint16 *stack = (uint16 *)(prev->ss * 16 + prev->esp);

        switch (addr[0])
        {
        case 0xF4: // hlt
            // Exit v8086 mode
            gdt[gp_tss.link / 8].sd_access &= ~0x2;
            
            gp_tss.link = prev->link;
            prev->link = 0;

            return;

        case 0xCD: // int
        {
            uint32 intno = addr[1];
            uint32 int_cs = ((uint16 *)0)[2 * intno + 1];
            uint32 int_ip = ((uint16 *)0)[2 * intno];

            // Save regs
            stack[-1] = prev->eflags;
            stack[-2] = prev->cs;
            stack[-3] = prev->eip + 2;

            prev->esp -= 6;

            prev->cs = int_cs;
            prev->eip = int_ip;

            return;
        }

        default:
            break;
        }
    }
    kprintf("GP# at 0x%x [%x:%x]\nCode: %x\n", link, cs, eip, error_code);
    panic("System halted");
}

void tssinit()
{
    // 4
    memset(&kernel_tss, sizeof(kernel_tss), 0);
    kernel_tss.link = 0;
    kernel_tss.iopb_offset = sizeof(tss_t);

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
    page_fault_tss.iopb_offset = sizeof(tss_t);

    // 7
    memset(&gp_tss, sizeof(gp_tss), 0);
    gp_tss.link = 0;
    gp_tss.eip = (uintptr)&GeneralProtectionHandler;

    asm("movl %%cr3, %%eax\n\t"
        : "=a"(gp_tss.cr3)
        :
        :);

    // disable interrupt
    asm("pushfl\n\t"
        "pop %%eax\n\t"
        "andl $0xfffffdff, %%eax"
        : "=a"(gp_tss.eflags)
        :
        :);

    gp_tss.esp = gp_tss.esp0 = (uintptr)page_fault_stack + 4080;

    gp_tss.ds =
        gp_tss.es =
            gp_tss.fs =
                gp_tss.gs = 0x10;
    gp_tss.cs = 0x8;
    gp_tss.ss = gp_tss.ss0 = 0x18;

    gp_tss.ldtr = 0;
    gp_tss.iopb_offset = sizeof(tss_t);

    asm("mov $0x20, %ax\n\t"
        "ltr %ax\n\t");

    int32 set_task_evec(uint32 xnum, uint32 handler);

    // Page fault
    set_task_evec(14, 0x28);

    // General Protection Fault
    set_task_evec(13, 0x38);
}
