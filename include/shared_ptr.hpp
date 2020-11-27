#ifndef _SHARED_PTR_HPP

#define _SHARED_PTR_HPP

#include <new.hpp>

template <typename ValueType>
class shared_ptr
{
    struct shared_buffer
    {
        ValueType *ptr;
        mutable int ref;

        // Not need

        int addRef() const
        {
            kprintf("Constructed");
            if (int ret = __sync_add_and_fetch(&ref, 1); ret < 0)
            {
                return 0;
            }
            else
                return ret;
        }

        int decRef()
        {
            kprintf("Destructed");
            int ret = __sync_sub_and_fetch(&ref, 1);
            if (__sync_bool_compare_and_swap(&ref, 0, 0x80000000))
            {
                return 0;
            }
            else
                return ret;
        }
    };

    HANDLE heap;
    shared_buffer *buffer;

public:
    shared_ptr(HANDLE heap) : buffer(nullptr), heap(heap) {}
    shared_ptr(HANDLE heap, ValueType *pvalue) : heap(heap)
    {
        buffer = new ((uintptr)heap) shared_buffer{pvalue, 1};
    }
    shared_ptr(const shared_ptr &s)
        : heap(s.heap), buffer(s.buffer)
    {
        if (buffer && buffer->addRef() == 0)
            buffer = nullptr;
    }
    ~shared_ptr()
    {
        shared_buffer *pbuffer = (shared_buffer *)__atomic_exchange_n(&buffer, 0, __ATOMIC_SEQ_CST);

        if (pbuffer)
        {
            if (pbuffer->decRef() == 0)
            {
                operator delete(pbuffer->ptr, (uintptr)heap);
                operator delete(pbuffer, (uintptr)heap);
            }
        }
    }

    shared_ptr &operator=(const shared_ptr &r)
    {
        this->~shared_ptr();
        new (this) shared_ptr(r);

        return *this;
    }

    ValueType &operator*()
    {
        return *(buffer->ptr);
    }

    ValueType *operator->()
    {
        return buffer->ptr;
    }
};

#endif
