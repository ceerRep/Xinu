#ifndef _VGA_H

#define _VGA_H

#include <xinu.h>

devcall vgawrite(
    struct dentry *devptr, /* Entry in device switch table */
    char *buff,            /* Buffer holding data to write	*/
    int32 count            /* Number of bytes to write	*/
);

devcall vgaseek(
    struct dentry *devptr, /* Entry in device switch table */
    uint32 offset          /* Byte position in the file	*/
);

devcall vgaputc(
    struct dentry *devptr, /* Entry in device switch table */
    char ch                /* Character (byte) to write	*/
);

devcall vgactrl(
    struct dentry *devptr, /* Entry in device switch table	*/
    int32 func,            /* Function to perform		*/
    int32 arg1,            /* Argument 1 for request	*/
    int32 arg2             /* Argument 2 for request	*/
);

enum
{
    VGA_MODESET,
    VGA_MODEGET
};

enum
{
    VGA_MODE_TEXT_80_25 = 3,
    VGA_MODE_GRAPHIC_320_200_256 = 0x13
};

#endif
