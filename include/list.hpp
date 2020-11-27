#ifndef _LIST_HPP

#define _LIST_HPP

#include "heap.h"
#include "new.hpp"

template <typename ValueType>
class List;

template <typename ValueType>
struct Node
{
    Node<ValueType> *prev, *next;
    alignas(ValueType) char value_buffer[sizeof(ValueType)];
};

template <typename ValueType>
class Iterator
{
    Node<ValueType> *node;

    Iterator(Node<ValueType> *node)
        : node(node)
    {
    }

    Node<ValueType> *getNode()
    {
        return node;
    }

public:
    friend class List<ValueType>;

    Iterator(const Iterator<ValueType> &) = default;

    Iterator<ValueType> &operator++()
    {
        node = node->next;
        return *this;
    }

    Iterator<ValueType> &operator--()
    {
        node = node->prev;
        return *this;
    }

    Iterator<ValueType> operator++(int)
    {
        Iterator<ValueType> ret{*this};
        node = node->next;
        return ret;
    }

    Iterator<ValueType> operator--(int)
    {
        Iterator<ValueType> ret{*this};
        node = node->prev;
        return ret;
    }

    ValueType &operator*()
    {
        return *(ValueType *)node->value_buffer;
    }

    const ValueType &operator*() const
    {
        return *(ValueType *)node->value_buffer;
    }

    ValueType *operator->()
    {
        return (ValueType *)node->value_buffer;
    }

    const ValueType *operator->() const
    {
        return (ValueType *)node->value_buffer;
    }

    bool operator==(const Iterator<ValueType> &r) const
    {
        return node == r.node;
    }

    bool operator!=(const Iterator<ValueType> &r) const
    {
        return node != r.node;
    }
};

template <typename _ValueType>
class List
{
    typedef Node<_ValueType> node_t;
    HANDLE heap;
    int m_size;
    node_t m_begin, m_end;

public:
    typedef Iterator<_ValueType> iterator;
    typedef _ValueType ValueType;
    List()
        : List(getDefaultHeap())
    {
    }

    List(HANDLE heap)
        : heap(heap), m_size(0)
    {
        m_begin.prev = &m_end;
        m_begin.next = &m_end;

        m_end.prev = &m_begin;
        m_end.next = &m_begin;
    }

    iterator insert(iterator it, ValueType value)
    {
        node_t *now = it.getNode();
        node_t *to_insert = (node_t *)HeapAlloc(heap, sizeof(node_t));

        if (to_insert)
            return end();

        to_insert->next = now->next;
        now->next->prev = to_insert;
        to_insert->prev = now;
        now->next = to_insert;

        m_size++;

        new ((void *)to_insert->value_buffer) ValueType(value);

        return iterator(to_insert);
    }

    iterator push_front(ValueType value)
    {
        return insert(iterator(&m_begin), value);
    }

    iterator push_back(ValueType value)
    {
        return insert(iterator(m_end.prev), value);
    }

    bool erase(iterator it)
    {
        node_t *now = it.getNode();
        if (now != &m_begin || now != &m_end)
        {
            now->prev->next = now->next;
            now->next->prev = now->prev;
            HeapFree(heap, now);

            m_size--;
            return true;
        }
        return false;
    }

    bool pop_back()
    {
        return erase(iterator(m_end.prev));
    }

    bool pop_front()
    {
        return erase(iterator(m_begin.next));
    }

    ValueType &front()
    {
        return *(ValueType *)(m_begin.next->value_buffer);
    }

    ValueType &back()
    {
        return *(ValueType *)(m_end.prev->value_buffer);
    }

    iterator begin()
    {
        return ++iterator(&m_begin);
    }

    iterator end()
    {
        return iterator(&m_end);
    }

    int size()
    {
        return m_size;
    }
};

#endif
