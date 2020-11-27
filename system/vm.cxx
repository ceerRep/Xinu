#include <xinu.h>

#include <new.hpp>
#include <IRQMaskGuard.hpp>

static_assert(sizeof(PDirEntryFatPage_t) == sizeof(PDirEntryTable_t), "Sizeof PDirEntryFatPage_t not equal to sizeof PDirEntryTable_t");

inline HANDLE hVMHeap;

extern "C"
{
    PageManager_t *PManager = NULL;
    extern char _end[0];
    extern void *maxheap;
    extern void *minheap;
}

static int vmLog = 0;

intptr PageAlloc()
{
    if (PManager->free_pages <= 0)
        return SYSERR;
    else
    {
        intptr ret = PManager->free_pages_list[--(PManager->free_pages)];

        if (vmLog)
            kprintf("Page allocated: %d\r\n", ret);

        return ret;
    }
}

void PageFree(intptr pgno)
{
    if (vmLog)
        kprintf("Page deallocated: %d\r\n", pgno);
    PManager->free_pages_list[(PManager->free_pages)++] = pgno;
}

intptr PageTableAlloc()
{
    if (PManager->free_pgtables <= 0)
        return SYSERR;
    else
    {
        intptr ret = PManager->free_pgtable_list[--(PManager->free_pgtables)];

        auto &pageTable = KERNEL_PGTABLE_AT(ret);

        for (auto &entry : pageTable)
        {
            entry.present = 0;
            entry.allow_user_access = 0;
            entry.allow_write = 1;
            entry.global = 0;
            entry.page_cache_disable = 0;
            entry.page_write_through = 0;
        }

        if (vmLog)
            kprintf("Page table allocated: %d\r\n", ret);

        return ret;
    }
}

void PageTableFree(intptr pgdirno)
{
    if (vmLog)
        kprintf("Page table deallocated: %d\r\n", pgdirno);
    PManager->free_pgtable_list[(PManager->free_pgtables)++] = pgdirno;
}

PageManager_t *pageManagerFactory()
{
    intptr pages = (uintptr)maxheap / PAGE_SIZE;

    PManager = new (sizeof(PageManager_t) +
                        sizeof(intptr) * pages +
                        sizeof(intptr) * KERNEL_PG_TABLE_NUM,
                    (uintptr)hVMHeap) PageManager_t;
    PManager->all_pages = pages;
    PManager->free_pages = 0;
    PManager->free_pgtables = 0;
    PManager->free_pages_list = (intptr *)PManager->buffer;
    PManager->free_pgtable_list = ((intptr *)PManager->buffer) + pages;

    for (uintptr now = KERNEL_PG_POS + KERNEL_PG_SIZE; now < (uintptr)maxheap; now += PAGE_SIZE)
    {
        PageFree(PAGENO(now));
    }

    for (uintptr now = 1; now < KERNEL_PG_TABLE_NUM; now++)
    {
        PageTableFree(now);
    }

    return PManager;
}

int EnsureAccess(PDirEntry_t *pg, uintptr va)
{
    if (!pg[PDX(va)].table.present)
    {
        ;
    }

    if (pg[PTX(va)].table.page_size_zero == 0)
    {
        ;
    }

    return 0;
}

extern "C"
{
    void vminit()
    {
        // Initialize Kernel Heap

        hVMHeap = HeapInitialize(kernel_pg_pos - kernel_vm_heap_start, kernel_vm_heap_start);

        PDirEntry_t *null_page_dir = KERNEL_PGDIR_AT(0);

        for (int i = 0; i < ENTRY_PER_PAGE; i++)
        {
            null_page_dir[i].table.present = 0;
            null_page_dir[i].table.page_size_zero = 0;
            null_page_dir[i].table.page_write_through = 0;
            null_page_dir[i].table.page_cache_disable = 0;
            null_page_dir[i].table.allow_write = 1;
            null_page_dir[i].table.allow_user_access = 0;
        }

        // Initialize Fat page
        for (uintptr now = 0; now < (uintptr)_end; now += FAT_PAGE_SIZE)
        {
            null_page_dir[PDX(now)].fat_page.address = PDX(now);
            null_page_dir[PDX(now)].fat_page.allow_user_access = 0;
            null_page_dir[PDX(now)].fat_page.allow_write = 1;
            null_page_dir[PDX(now)].fat_page.global = 1;
            null_page_dir[PDX(now)].fat_page.page_cache_disable = 0;
            null_page_dir[PDX(now)].fat_page.page_size_one = 1;
            null_page_dir[PDX(now)].fat_page.page_write_through = 0;
            null_page_dir[PDX(now)].fat_page.pat_zero = 0;
            null_page_dir[PDX(now)].fat_page.present = 1;
            null_page_dir[PDX(now)].fat_page.reserved_zero = 0;
        }

        // Initialize page manager

        PageManager_t *PManager = pageManagerFactory();

        // VM initialized
        vmLog = 1;

        // Enable Paging
        asm("mov %%eax, %%cr3\n\t"
            "mov %%cr4, %%eax\n\t"
            // Enable 4MB Page
            "or  $0x00000010, %%eax\n\t"
            "mov %%eax, %%cr4\n\t"
            // Enable Paging
            "mov %%cr0, %%eax\n\t"
            "or  $0x80000001, %%eax\n\t"
            "mov %%eax, %%cr0\n\t"
            :
            : "a"(null_page_dir)
            :);
    }

    void segmentIncRef(VirtualSegment *segment)
    {
        segment->refs++;
    }

    void segmentDecRef(VirtualSegment *segment)
    {
        segment->refs--;

        if (segment->refs == 0)
        {
            uintptr pageNum = segment->segmentSize / PAGE_SIZE;
            uintptr pageDirNum = ALIGN_CEIL(pageNum, ENTRY_PER_PAGE) / ENTRY_PER_PAGE;

            for (int i = 0; i < pageDirNum; i++)
                if (segment->pageTables[i])
                {
                    auto &pageTable = KERNEL_PGTABLE_AT(segment->pageTables[i]);
                    for (auto page : pageTable)
                        if (page.present)
                            PageFree(page.address);
                    PageTableFree(segment->pageTables[i]);
                }

            operator delete(segment, (uintptr)hVMHeap);
        }
    }

    void vmEnterProcess(pid32 pid)
    {
        IRQMaskGuard disable_interrupt;
        // kprintf("Enter %d\r\n", pid);
        auto &nowProcent = proctab[pid];

        for (auto &segment : nowProcent.procVMInfo->segments)
            if (segment.segment)
                segmentIncRef(segment.segment);
    }

    void vmLeaveProcess(pid32 pid)
    {
        IRQMaskGuard disable_interrupt;
        // kprintf("Leave %d\r\n", pid);
        auto &nowProcent = proctab[pid];

        for (auto &segment : nowProcent.procVMInfo->segments)
            if (segment.segment)
                segmentDecRef(segment.segment);
    }

    void *createNewSegment(uintptr size, void *va)
    {
        return createNewSegmentTo(currpid, size, va);
    }

    void *createNewSegmentTo(pid32 pid, uintptr size, void *va)
    {
        // Disable Interrupt
        IRQMaskGuard disable_interrupt;

        size = ALIGN_CEIL(size, FAT_PAGE_SIZE);
        va = (void *)ALIGN_FLOOR(va, FAT_PAGE_SIZE);

        uintptr pageNum = size / PAGE_SIZE;
        uintptr pageDirNum = ALIGN_CEIL(pageNum, ENTRY_PER_PAGE) / ENTRY_PER_PAGE;

        auto &nowProcent = proctab[pid];
        ProcessVMMappingRecord_t *record = NULL;

        for (int i = 0; i < PROC_SEGMENT_COUNT; i++)
            if (nowProcent.procVMInfo->segments[i].segment == 0)
            {
                record = &nowProcent.procVMInfo->segments[i];
                break;
            }

        if (record == NULL)
            return NULL;

        record->virtual_address = va;
        record->segment = new (sizeof(VirtualSegment_t) + sizeof(intptr) * pageDirNum,
                               (uintptr)hVMHeap) VirtualSegment_t;

        // Ref by this process | Ref by running process
        record->segment->refs = pid == currpid ? 2 : 1;
        record->segment->segmentSize = size;

        for (int i = 0; i < pageDirNum; i++)
            record->segment->pageTables[i] = NULL;

        return va;
    }

    void *attachRemoteSegment(pid32 rpid, void *la, void *ra)
    {
        // Disable Interrupt
        IRQMaskGuard disable_interrupt;

        la = (void *)ALIGN_FLOOR(la, FAT_PAGE_SIZE);
        ra = (void *)ALIGN_FLOOR(ra, FAT_PAGE_SIZE);

        auto &targetProcent = proctab[currpid];
        auto &sourceProcent = proctab[rpid];

        for (auto &seg : sourceProcent.procVMInfo->segments)
            if (seg.segment && seg.virtual_address == ra)
            {
                for (auto &newseg : targetProcent.procVMInfo->segments)
                    if (newseg.segment == NULL)
                    {
                        newseg.segment = seg.segment;

                        // Twice
                        segmentIncRef(seg.segment);
                        segmentIncRef(seg.segment);
                        newseg.virtual_address = la;

                        return la;
                    }
                return NULL;
            }

        return NULL;
    }

    int detachSegment(void *va)
    {
        return detachSegmentFrom(currpid, va);
    }

    int detachSegmentFrom(pid32 pid, void *va)
    {
        // Disable Interrupt
        IRQMaskGuard disable_interrupt;

        auto &nowProcent = proctab[pid];
        va = (void *)ALIGN_FLOOR(va, FAT_PAGE_SIZE);

        for (int i = 0; i < PROC_SEGMENT_COUNT; i++)
            if (nowProcent.procVMInfo->segments[i].segment &&
                nowProcent.procVMInfo->segments[i].virtual_address == va)
            {
                auto &segment = nowProcent.procVMInfo->segments[i].segment;
                uintptr pageNum = segment->segmentSize / PAGE_SIZE;
                uintptr pageDirNum = ALIGN_CEIL(pageNum, ENTRY_PER_PAGE) / ENTRY_PER_PAGE;

                for (int i = PDX(va); i < PDX(va) + pageDirNum; i++)
                {
                    if (auto &table = (KERNEL_PGDIR_AT(nowProcent.procVMInfo->pgDirNo)[i]).table; table.present)
                    {
                        if (pid == currpid)
                            for (int j = 0; j < ENTRY_PER_PAGE; j++)
                            {
                                invlpg((uintptr)va + i * FAT_PAGE_SIZE + j * PAGE_SIZE);
                            }
                        table.present = 0;
                    }
                }

                segmentDecRef(segment);

                // current ref
                if (pid == currpid)
                    segmentDecRef(segment);

                nowProcent.procVMInfo->segments[i].segment = NULL;

                return OK;
            }

        return SYSERR;
    }

    ProcessVMInfo *processVMInfoFactory(ProcessVMInfo *parent)
    {
        ProcessVMInfo *info = new ((uintptr)hVMHeap) ProcessVMInfo;

        if (info == NULL)
            panic("Error allocting vminfo");

        info->pgDirNo = PageTableAlloc();

        memcpy(&KERNEL_PGDIR_AT(info->pgDirNo),
               &KERNEL_PGDIR_AT(parent ? parent->pgDirNo : 0),
               PAGE_SIZE);

        for (int i = 0; i < PROC_SEGMENT_COUNT; i++)
            if (parent)
            {
                info->segments[i] = parent->segments[i];
                if (info->segments[i].segment)
                    segmentIncRef(info->segments[i].segment);
            }
            else
                info->segments[i].segment = NULL;

        return info;
    }

    void freeVMInfo(ProcessVMInfo_t *info)
    {
        IRQMaskGuard disable_interrupt;
        PageTableFree(info->pgDirNo);
        operator delete(info, (uintptr)hVMHeap);
    }

    void PageFaultHandler()
    {
        asm("1:\n\t"
            "call PageFaultHandler_\n\t"
            "pop %eax\n\t" // unused error code
            "iret\n\t"
            "jmp 1b\n\t");
    }

    void PageFaultHandler_()
    {
        IRQMaskGuard disable_interrupt;
        uintptr addr, pteaddr;

        auto &proc = proctab[currpid];

        kernel_tss.cr3 = (uintptr)&KERNEL_PGDIR_AT(proc.procVMInfo->pgDirNo);

        asm("mov %%eax, %%cr3\n\t"
            :
            : "a"(&KERNEL_PGDIR_AT(proc.procVMInfo->pgDirNo))
            :);

        asm("mov %%cr2, %0\n\t"
            : "=r"(addr)
            :
            :);

        for (auto &segment : proc.procVMInfo->segments)
        {
            if (segment.segment &&
                addr >= (uintptr)segment.virtual_address &&
                addr < (uintptr)segment.virtual_address + segment.segment->segmentSize)
            {
                uintptr pdx = PDX(addr);
                uintptr pd_offset = pdx - PDX(segment.virtual_address);
                auto &procPG = KERNEL_PGDIR_AT(proc.procVMInfo->pgDirNo);

                if (!segment.segment->pageTables[pd_offset])
                {
                    segment.segment->pageTables[pd_offset] = PageTableAlloc();

                    if (segment.segment->pageTables[pd_offset] == SYSERR)
                    {
                        if (vmLog)
                            kprintf("Page fault: page limit exceeded\r\n");
                        panic("System halted");
                    }
                }
                procPG[pdx].table.address = (uintptr)&KERNEL_PGTABLE_AT(segment.segment->pageTables[pd_offset]) >> PTXSHIFT;
                procPG[pdx].table.present = 1;

                uintptr ptx = PTX(addr);

                auto &pgTable = KERNEL_PGTABLE_AT(segment.segment->pageTables[pd_offset]);

                if (!pgTable[ptx].present)
                {
                    pgTable[ptx].address = PageAlloc();
                    pgTable[ptx].present = 1;
                }

                if (vmLog)
                    kprintf("Page fault: %x -> %x\r\n", pgTable[ptx].address, addr);

                // Maybe not need
                invlpg(addr);

                return;
            }
        }

        if (vmLog)
            kprintf("Page fault: %x\r\n", addr);
        panic("System halted");
    }
}
