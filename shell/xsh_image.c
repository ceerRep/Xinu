#include <xinu.h>

extern char xinu_image[0];

asm(".global xinu_image\n\t"
    "xinu_image:\n\t"
    ".incbin \"../shell/xinu.bin\"");

shellcmd xsh_image(int nargs, char *args[])
{
    control(KBD0, KBD_SET_BLOCKING, 0, 0);
    while (getc(KBD0) != EOF)
        ;
    control(KBD0, KBD_SET_BLOCKING, 1, 0);

    control(VGA0, VGA_MODESET, VGA_MODE_GRAPHIC_320_200_256, 0);

    for (int i = 0; i < 64000; i++)
        putc(VGA0, xinu_image[i]);

    getc(KBD0);

    control(VGA0, VGA_MODESET, VGA_MODE_TEXT_80_25, 0);
    return 0;
}
