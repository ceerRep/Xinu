#include <xinu.h>

#include <icu.h>

#define KBD_BUFFER_SIZE 256

static char buffer[KBD_BUFFER_SIZE];
static uintptr head = 0, tail = 0;
static int kbd_echo = 1, kbd_blocking = 1;

static sid32 kbdsem;

static uint8 shiftcode[256] =
    {
        [0x1D] = CTL,
        [0x2A] = SHIFT,
        [0x36] = SHIFT,
        [0x38] = ALT,
        [0x9D] = CTL,
        [0xB8] = ALT};

static uint8 togglecode[256] =
    {
        [0x3A] = CAPSLOCK,
        [0x45] = NUMLOCK,
        [0x46] = SCROLLLOCK};

static uint8 normalmap[256] =
    {
        NO, 0x1B, '1', '2', '3', '4', '5', '6', // 0x00
        '7', '8', '9', '0', '-', '=', '\b', '\t',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', // 0x10
        'o', 'p', '[', ']', '\n', NO, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', // 0x20
        '\'', '`', NO, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', NO, '*', // 0x30
        NO, ' ', NO, NO, NO, NO, NO, NO,
        NO, NO, NO, NO, NO, NO, NO, '7', // 0x40
        '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', NO, NO, NO, NO, // 0x50
        [0x9C] = '\n',                      // KP_Enter
        [0xB5] = '/',                       // KP_Div
        [0xC8] = KEY_UP, [0xD0] = KEY_DN,
        [0xC9] = KEY_PGUP, [0xD1] = KEY_PGDN,
        [0xCB] = KEY_LF, [0xCD] = KEY_RT,
        [0x97] = KEY_HOME, [0xCF] = KEY_END,
        [0xD2] = KEY_INS, [0xD3] = KEY_DEL};

static uint8 shiftmap[256] =
    {
        NO, 033, '!', '@', '#', '$', '%', '^', // 0x00
        '&', '*', '(', ')', '_', '+', '\b', '\t',
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', // 0x10
        'O', 'P', '{', '}', '\n', NO, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', // 0x20
        '"', '~', NO, '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', NO, '*', // 0x30
        NO, ' ', NO, NO, NO, NO, NO, NO,
        NO, NO, NO, NO, NO, NO, NO, '7', // 0x40
        '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', NO, NO, NO, NO, // 0x50
        [0x9C] = '\n',                      // KP_Enter
        [0xB5] = '/',                       // KP_Div
        [0xC8] = KEY_UP, [0xD0] = KEY_DN,
        [0xC9] = KEY_PGUP, [0xD1] = KEY_PGDN,
        [0xCB] = KEY_LF, [0xCD] = KEY_RT,
        [0x97] = KEY_HOME, [0xCF] = KEY_END,
        [0xD2] = KEY_INS, [0xD3] = KEY_DEL};

static uint8 ctlmap[256] =
    {
        NO, NO, NO, NO, NO, NO, NO, NO,
        NO, NO, NO, NO, NO, NO, NO, NO,
        C('Q'), C('W'), C('E'), C('R'), C('T'), C('Y'), C('U'), C('I'),
        C('O'), C('P'), NO, NO, '\r', NO, C('A'), C('S'),
        C('D'), C('F'), C('G'), C('H'), C('J'), C('K'), C('L'), NO,
        NO, NO, NO, C('\\'), C('Z'), C('X'), C('C'), C('V'),
        C('B'), C('N'), C('M'), NO, NO, C('/'), NO, NO,
        [0x9C] = '\r',   // KP_Enter
        [0xB5] = C('/'), // KP_Div
        [0xC8] = KEY_UP, [0xD0] = KEY_DN,
        [0xC9] = KEY_PGUP, [0xD1] = KEY_PGDN,
        [0xCB] = KEY_LF, [0xCD] = KEY_RT,
        [0x97] = KEY_HOME, [0xCF] = KEY_END,
        [0xD2] = KEY_INS, [0xD3] = KEY_DEL};

static devcall kbd_raw_getc()
{
    static uint32 shift;
    static uint8 *charcode[4] = {
        normalmap, shiftmap, ctlmap, ctlmap};
    uint32 st, data, c;

    st = inb(KBSTATP);
    if ((st & KBS_DIB) == 0)
        return -1;
    data = inb(KBDATAP);

    if (data == 0xE0)
    {
        shift |= E0ESC;
        return 0;
    }
    else if (data & 0x80)
    {
        // Key released
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    }
    else if (shift & E0ESC)
    {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];
    if (shift & CAPSLOCK)
    {
        if ('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if ('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }

    return c;
}

devcall kbdinit(
    struct dentry *devptr /* Entry in device switch table */
)
{
    kbdsem = semcreate(0);
    set_evec(devptr->dvirq, (uint32)kbddisp);
    return OK;
}

devcall kbdread(
    struct dentry *devptr, /* Entry in device switch table */
    char *buff,            /* Buffer holding data to write	*/
    int32 count            /* Number of bytes to write	*/
)
{
    for (int i = 0; i < count; i++)
        buff[i] = devptr->dvgetc(devptr);

    return OK;
}

devcall kbdgetc(
    struct dentry *devptr /* Entry in device switch table */
)
{
    char ret;

    if (!kbd_blocking)
    {
        intmask mask = disable();
        if (semcount(kbdsem) > 0)
        {
            wait(kbdsem);
            restore(mask);
        }
        else
        {
            restore(mask);
            return EOF;
        }
    }
    else
        wait(kbdsem);

    intmask mask = disable();

    ret = buffer[head++];
    head = head % KBD_BUFFER_SIZE;

    restore(mask);

    return ret;
}

devcall kbdctrl(
    struct dentry *devptr, /* Entry in device switch table	*/
    int32 func,            /* Function to perform		*/
    int32 arg1,            /* Argument 1 for request	*/
    int32 arg2             /* Argument 2 for request	*/
)
{
    switch (func)
    {
        case KBD_SET_ECHO:
            kbd_echo = arg1;
            break;
        
        case KBD_GET_ECHO:
            return kbd_echo;
        
        case KBD_SET_BLOCKING:
            kbd_blocking = arg1;
            break;
        
        case KBD_GET_BLOCKING:
            return kbd_blocking;
        
        default:
            return SYSERR;
    }

    return OK;
}

asm(".global kbddisp\n\t"
    "kbddisp:\n\t"
    "pushal\n\t"
    "call kbddisp_\n\t"
    "popal\n\t"
    "iret");

void kbddisp_()
{
    outb(ICU1, EOI);
    char ch;

    while ((ch = kbd_raw_getc()) > 0)
    {
        if (kbd_echo && control(VGA0, VGA_MODEGET, 0, 0) == VGA_MODE_TEXT_80_25)
        {
            if (ch == '\n')
                putc(VGA0, '\r');
            putc(VGA0, ch);

            if (ch == '\b')
            {
                putc(VGA0, ' ');
                putc(VGA0, '\b');
            }
        }

        if ((tail + 1) % KBD_BUFFER_SIZE != head)
        {
            buffer[tail++] = head;
            tail %= 256;
            resched_cntl(DEFER_START);
            signal(kbdsem);
            resched_cntl(DEFER_STOP);
        }
    }
}
