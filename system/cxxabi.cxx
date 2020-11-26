#include "xinu.h"

#include "list.hpp"

extern "C++"
{
    void *operator new(uintptr size) noexcept
    {
        return HeapAlloc(getDefaultHeap(), size);
    }
    void *operator new[](uintptr size) noexcept
    {
        return HeapAlloc(getDefaultHeap(), size);
    }
    void operator delete(void *ptr) noexcept
    {
        HeapFree(getDefaultHeap(), ptr);
    }
    void operator delete[](void *ptr) noexcept
    {
        HeapFree(getDefaultHeap(), ptr);
    }

    void *operator new(uintptr size, uintptr heap) noexcept
    {
        return HeapAlloc((HANDLE)heap, size);
    }

    void *operator new(uintptr size, uintptr size1, uintptr heap) noexcept
    {
        return HeapAlloc((HANDLE)heap, size1);
    }

    void operator delete(void *ptr, uintptr heap) noexcept
    {
        HeapFree((HANDLE)heap, ptr);
    }

    // Default placement versions of operator new.
    inline void *operator new(uintptr, void *__p) noexcept
    {
        return __p;
    }
    inline void *operator new[](uintptr, void *__p) noexcept
    {
        return __p;
    }

    // Default placement versions of operator delete.
    inline void operator delete(void *, void *) noexcept {}
    inline void operator delete[](void *, void *) noexcept {}
    //@}
}

extern "C" void __cxa_pure_virtual()
{
    panic("Pure virtual function called");
}

namespace
{
    void __attribute__((constructor)) _constructor_test()
    {
        kprintf("Constructor called\n");
    }
} // namespace

extern "C" void naive()
{
    List<int> lst;
    lst.push_back(1);
    lst.push_back(2);
    lst.push_back(3);

    for (auto &i : lst)
        kprintf("%d %x\n", i, &i);

    while (lst.size())
        lst.pop_back();

    lst.push_back(5);
    lst.push_back(6);
    lst.push_back(7);

    for (auto &i : lst)
        kprintf("%d %x\n", i, &i);
}
