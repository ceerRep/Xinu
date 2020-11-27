// The -*- C++ -*- dynamic memory management header.

// Copyright (C) 1994-2020 Free Software Foundation, Inc.

// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file new
 *  This is a Standard C++ Library header.
 *
 *  The header @c new defines several functions to manage dynamic memory and
 *  handling memory allocation errors; see
 *  https://gcc.gnu.org/onlinedocs/libstdc++/manual/dynamic_memory.html
 *  for more.
 */

#ifndef _NEW
#define _NEW

#include <xinu.h>

#pragma GCC system_header

#pragma GCC visibility push(default)

extern "C++"
{

    //@{
    /** These are replaceable signatures:
 *  - normal single new and delete (no arguments, throw @c bad_alloc on error)
 *  - normal array new and delete (same)
 *  - @c nothrow single new and delete (take a @c nothrow argument, return
 *    @c NULL on error)
 *  - @c nothrow array new and delete (same)
 *
 *  Placement new and delete signatures (take a memory address argument,
 *  does nothing) may not be replaced by a user's program.
*/
    void *operator new(uintptr size) noexcept;
    void *operator new[](uintptr size) noexcept;
    void operator delete(void *ptr) noexcept;
    void operator delete[](void *ptr) noexcept;

    void *operator new(uintptr size, uintptr heap) noexcept;
    void *operator new(uintptr size, uintptr size1, uintptr heap) noexcept;
    void operator delete(void *ptr, uintptr heap) noexcept;

    // Default placement versions of operator new.
    inline void *operator new(uintptr, void *__p) noexcept;
    inline void *operator new[](uintptr, void *__p) noexcept;

    // Default placement versions of operator delete.
    inline void operator delete(void *, void *) noexcept;
    inline void operator delete[](void *, void *) noexcept;
    //@}

} // extern "C++"

#pragma GCC visibility pop

#endif
