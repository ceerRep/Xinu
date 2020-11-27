#include <xinu.h>

#define MIN_NODE_SIZE (sizeof(HeapNode_t) + 4)

HANDLE hKernelHeap = 0;

// TODO:

#define AVAIL_MAGIC 0x55AA55AA
#define USED_MAGIC 0xAA55AA55

typedef struct HeapNode
{
    uintptr size;
    uintptr free;
    char payload[0];
    struct HeapNode *next;
} HeapNode_t;

typedef struct HeapInfo
{
    uintptr heapSize;
    HeapNode_t *avail_first;
} HeapInfo_t;

HANDLE HeapInitialize(uintptr size, void *virtualMem)
{
    // TODO: assert mininum size

    HeapInfo_t *info = virtualMem;
    HeapNode_t *first = (HeapNode_t *)(info + 1);

    first = (void *)ALIGN_CEIL(first, 4);

    info->heapSize = size;
    info->avail_first = first;

    first->size = virtualMem + size - (void *)first;
    first->free = AVAIL_MAGIC;
    first->next = 0;
    return virtualMem;
}

void *HeapAlloc(HANDLE hHeap, uintptr size)
{
    HeapInfo_t *heap = hHeap;
    size = ALIGN_CEIL(size, 4) + offsetof(HeapNode_t, payload);

    for (HeapNode_t *now = heap->avail_first, *prev = NULL; now; prev = now, now = now->next)
    {
        if (now->free != AVAIL_MAGIC)
            panic("Unexpected magic at heapalloc");

        if (now->size >= size)
        {
            HeapNode_t *next;
            if (now->size - size >= MIN_NODE_SIZE) // Split
            {
                HeapNode_t *newnode = (void *)now + size;
                newnode->size = now->size - size;
                newnode->next = now->next;
                newnode->free = AVAIL_MAGIC;

                now->size = size;

                next = newnode;
            }
            else
            {
                next = now->next;
            }
            if (prev)
            {
                prev->next = next;
            }
            else
            {
                heap->avail_first = next;
            }
            now->free = USED_MAGIC;
            return now->payload;
        }
    }

    return NULL;
}

void HeapFree(HANDLE hHeap, void *mem)
{
    HeapInfo_t *heap = hHeap;
    HeapNode_t *node = mem - offsetof(HeapNode_t, payload);

    if (node->free != USED_MAGIC)
        panic("Double free");

    node->free = AVAIL_MAGIC;

    for (HeapNode_t *now = heap->avail_first, *prev = NULL, **ppnow = &heap->avail_first;
         now || ppnow;
         ppnow = now ? &now->next : NULL, prev = now, now = now ? now->next : NULL)
    {
        if (!now || node < now)
        {
            node->next = now;

            if (prev)
            {
                prev->next = node;

                if ((void *)prev + prev->size == node)
                {
                    // Merge prev
                    prev->size += node->size;
                    node = prev;
                }
            }
            else
            {
                heap->avail_first = node;
            }

            // Normally it's true -> now > node -> now is not NULL
            if ((void *)node + node->size == now)
            {
                node->next = now->next;
                node->size += now->size;
            }

            return;
        }
    }
}

HANDLE getDefaultHeap()
{
    return proctab[currpid].procDefaultHeap;
}

HANDLE setDefaultHeap(HANDLE heap)
{
    HANDLE ret = proctab[currpid].procDefaultHeap;
    proctab[currpid].procDefaultHeap = heap;
    return ret;
}
