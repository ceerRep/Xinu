#include <xinu.h>

extern "C"
{
    extern char VGA_BASE[0];
    extern char VGA_TEXT_BASE[0];

    static int curr_cursor_pos = 0;
    static int curr_vga_mode = VGA_MODE_TEXT_80_25;

    devcall vgawrite(
        struct dentry *devptr, /* Entry in device switch table */
        char *buff,            /* Buffer holding data to write	*/
        int32 count            /* Number of bytes to write	*/
    )
    {
        for (int i = 0; i < count; i++)
            devptr->dvputc(devptr, buff[i]);
        return OK;
    }

    devcall vgaseek(
        struct dentry *devptr, /* Entry in device switch table */
        uint32 offset          /* Byte position in the file	*/
    )
    {
        if (curr_vga_mode == VGA_MODE_TEXT_80_25)
        {
            if (offset >= 80 * 25)
                return SYSERR;

            outb(0x3D4, 0x0F);
            outb(0x3D5, (uint8)(offset & 0xFF));
            outb(0x3D4, 0x0E);
            outb(0x3D5, (uint8)((offset >> 8) & 0xFF));
        }

        curr_cursor_pos = offset;

        return OK;
    }

    devcall vgaputc(
        struct dentry *devptr, /* Entry in device switch table */
        char ch                /* Character (byte) to write	*/
    )
    {
        if (curr_vga_mode == VGA_MODE_TEXT_80_25)
        {
            v8086_call(code16_source_int10,
                       (0x0e << 8) + ch,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0);
        }
        else
        {
            VGA_BASE[curr_cursor_pos++] = ch;
        }

        return OK;
    }

    devcall vgactrl(
        struct dentry *devptr, /* Entry in device switch table	*/
        int32 func,            /* Function to perform		*/
        int32 arg1,            /* Argument 1 for request	*/
        int32 arg2             /* Argument 2 for request	*/
    )
    {
        switch (func)
        {
        case VGA_MODESET:

            if (arg1 != VGA_MODE_TEXT_80_25 && arg1 != VGA_MODE_GRAPHIC_320_200_256)
                return SYSERR;

            v8086_call(code16_source_int10,
                       (0 << 8) + arg1,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0,
                       0);
            curr_vga_mode = arg1;
            curr_cursor_pos = 0;

            if (arg1 == VGA_MODE_TEXT_80_25)
                devptr->dvseek(devptr, 0);

            return OK;
        
        case VGA_MODEGET:
            return curr_vga_mode;

        default:
            return SYSERR;
        }
    }
}
