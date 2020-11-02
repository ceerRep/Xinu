#ifndef _SCHE_H

#define _SCHE_H

#include "conf.h"
#include "kernel.h"
#include "process.h"

#define timeslice_from_priority(prio)                                          \
    ({                                                                         \
        tim8 slice;                                                            \
        switch (prio)                                                          \
        {                                                                      \
        case -127 ... 0:                                                       \
            slice = -1;                                                        \
            break;                                                             \
        case 1 ... 10:                                                         \
            slice = 10;                                                        \
            break;                                                             \
        case 11 ... 20:                                                        \
            slice = 20;                                                        \
            break;                                                             \
        default:                                                               \
            slice = 30;                                                        \
        }                                                                      \
        slice;                                                                 \
    })

#endif
