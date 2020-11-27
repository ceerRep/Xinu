#ifndef _VM_H

#define _VM_H

/*
 * Lower  1GB: Kernel Space, Same for all process
 * Higher 3GB: User Space
 */

/*
 * 0 - _end + 4KiB: always direct mapping
 */

typedef struct PDirEntryFatPage
{
    unsigned present : 1;
    unsigned allow_write : 1;
    unsigned allow_user_access : 1;
    unsigned page_write_through : 1;
    unsigned page_cache_disable : 1;
    unsigned accessed : 1;
    unsigned dirty : 1;
    unsigned page_size_one : 1;
    unsigned global : 1;
    unsigned system_defined : 3;
    unsigned pat_zero : 1;
    unsigned reserved_zero : 9;
    unsigned address : 10;
} PDirEntryFatPage_t;

typedef struct PDirEntryTable
{
    unsigned present : 1;
    unsigned allow_write : 1;
    unsigned allow_user_access : 1;
    unsigned page_write_through : 1;
    unsigned page_cache_disable : 1;
    unsigned accessed : 1;
    unsigned system_defined_0 : 1;
    unsigned page_size_zero : 1;
    unsigned system_defined_1 : 4;
    unsigned address : 20;
} PDirEntryTable_t;

typedef union PDirEntry
{
    PDirEntryFatPage_t fat_page;
    PDirEntryTable_t table;
} PDirEntry_t;

typedef struct
{
    unsigned present : 1;
    unsigned allow_write : 1;
    unsigned allow_user_access : 1;
    unsigned page_write_through : 1;
    unsigned page_cache_disable : 1;
    unsigned accessed : 1;
    unsigned dirty : 1;
    unsigned pat_zero : 1;
    unsigned global : 1;
    unsigned system_defined : 3;
    unsigned address : 20;
} PTableEntry_t;

#define ENTRY_PER_PAGE ((signed)(PAGE_SIZE / sizeof(PDirEntryTable_t)))

// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

// page directory index
#define PDX(va) (((uint32)(va) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(va) (((uint32)(va) >> PTXSHIFT) & 0x3FF)

// pageno
#define PAGENO(a) ((uintptr)(a) >> PTXSHIFT)

// construct virtual address from indexes and offset
#define PGADDR(d, t, o) ((uint32)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// Page directory and page table constants.
#define NPDENTRIES 1024 // # directory entries per page directory
#define NPTENTRIES 1024 // # PTEs per page table
#define PGSIZE 4096 // bytes mapped by a page

#define PTXSHIFT 12 // offset of PTX in a linear address
#define PDXSHIFT 22 // offset of PDX in a linear address

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

#define PTE_P 0x0001 // present
#define PTE_W 0x002 // Writeable
#define PTE_ADDR(pte) ((uint32)(pte) & ~0xFFF)

// 4M
extern char kernel_pg_pos[0], kernel_vm_heap_start[0];
#define KERNEL_PG_POS ((uintptr)kernel_pg_pos)
#define KERNEL_PGDIR_AT(x) (((PageDir_t *)KERNEL_PG_POS)[x])
#define KERNEL_PGTABLE_AT(x) (((PageTable_t *)KERNEL_PG_POS)[x])
#define KERNEL_PG_SIZE 0x400000
#define KERNEL_PG_TABLE_NUM (KERNEL_PG_SIZE / PAGE_SIZE)

// 512M
#define KERNEL_VM_HEAP_START 0x20000000
#define KERNEL_HEAP_SIZE 0x20000000
#define KERNEL_HEAP_HANDLE ((void *)KERNEL_VM_HEAP_START)

#define PROC_SEGMENT_COUNT 10

#define invlpg(va)                 \
    do                             \
    {                              \
        asm volatile("invlpg (%0)" \
                     :             \
                     : "r"((va))   \
                     : "memory");  \
    } while (0)

typedef PDirEntry_t PageDir_t[ENTRY_PER_PAGE];
typedef PTableEntry_t PageTable_t[ENTRY_PER_PAGE];

typedef struct PageManager
{
    intptr all_pages;
    intptr free_pages;
    intptr free_pgtables;
    intptr *free_pages_list;
    intptr *free_pgtable_list;
    char buffer[];
} PageManager_t;

extern PageManager_t *PManager;

typedef struct VirtualSegment
{
    int refs;
    uintptr segmentSize;
    intptr pageTables[];
} VirtualSegment_t;

typedef struct ProcessVMMappingRecord
{
    void *virtual_address;
    VirtualSegment_t *segment;
} ProcessVMMappingRecord_t;

typedef struct ProcessVMInfo
{
    uintptr pgDirNo;
    ProcessVMMappingRecord_t segments[PROC_SEGMENT_COUNT];
} ProcessVMInfo_t;

void vminit();
void vmEnterProcess(pid32 pid);
void vmLeaveProcess(pid32 pid);
void segmentIncRef(VirtualSegment_t *segment);
void segmentDecRef(VirtualSegment_t *segment);
void *createNewSegment(uintptr size, void *va);
void *createNewSegmentTo(pid32 pid, uintptr size, void *va);
void *attachRemoteSegment(pid32 rpid, void *la, void *ra);
int detachSegment(void *va);
int detachSegmentFrom(pid32 pid, void *va);
ProcessVMInfo_t *processVMInfoFactory(ProcessVMInfo_t *parent);
void freeVMInfo(ProcessVMInfo_t *info);

#endif
