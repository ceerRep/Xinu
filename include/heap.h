#ifndef _HEAP_H

#define _HEAP_H

#include <xinu.h>

extern HANDLE hKernelHeap;

HANDLE HeapCreate(uintptr size);
HANDLE HeapCreateEx(uintptr size, void* virtualMem);
HANDLE HeapInitialize(uintptr size, void* virtualMem);
void HeapDestroy(HANDLE heap);

void* HeapAlloc(HANDLE heap, uintptr size);
void  HeapFree(HANDLE heap, void* mem);

HANDLE getDefaultHeap();
HANDLE setDefaultHeap(HANDLE);

#endif
