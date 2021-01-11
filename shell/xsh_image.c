#include <xinu.h>

extern char megumin_bitmap[0];

asm(".global megumin_bitmap\n\t"
    "megumin_bitmap:\n\t"
    ".incbin \"../shell/megumin_256.bin\"");

shellcmd xsh_image(int nargs, char *args[])
{
    control(KBD0, KBD_SET_BLOCKING, 0, 0);
    while (getc(KBD0) != EOF)
        ;
    control(KBD0, KBD_SET_BLOCKING, 1, 0);

    control(VGA0, VGA_MODESET, VGA_MODE_GRAPHIC_320_200_256, 0);

    char *megumin_palatte = megumin_bitmap;
    char *megumin_image = megumin_bitmap + 3 * 256;

    for (int i = 0; i < 256; i++)
        control(VGA0, VGA_PALETTE_SET, i,
                (megumin_palatte[i * 3] << 16) +
                    (megumin_palatte[i * 3 + 1] << 8) +
                    megumin_palatte[i * 3 + 2]);

    for (int i = 0; i < 64000; i++)
        putc(VGA0, megumin_image[i]);

    getc(KBD0);

    control(VGA0, VGA_MODESET, VGA_MODE_TEXT_80_25, 0);
    return 0;
}
