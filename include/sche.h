#ifndef _SCHE_H

#define _SCHE_H

#include "conf.h"
#include "kernel.h"
#include "process.h"

extern int kcycle_per_tick;

#define timeslice_from_priority(prio)                                          \
    ({                                                                         \
        tim32 slice;                                                           \
        switch (prio)                                                          \
        {                                                                      \
        case -127 ... 0:                                                       \
            slice = -1;                                                        \
            break;                                                             \
        case 1 ... 10:                                                         \
            slice = 10 * kcycle_per_tick;                                       \
            break;                                                             \
        case 11 ... 20:                                                        \
            slice = 20 * kcycle_per_tick;                                       \
            break;                                                             \
        default:                                                               \
            slice = 30 * kcycle_per_tick;                                       \
        }                                                                      \
        slice;                                                                 \
    })

#endif
