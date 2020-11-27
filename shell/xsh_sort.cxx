/* xsh sort */

#include <xinu.h>
#include <new.hpp>

#define HEAP_POS 0x10000000
#define HEAP_SIZE 8192

namespace
{
    void merge_sort(int n, char **args);
}
/*------------------------------------------------------------------------
 * xsh_sort  -  Shell command to sort args
 *------------------------------------------------------------------------
 */
extern "C" shellcmd xsh_sort(int nargs, char *args[])
{
    if (nargs == 2 && strncmp(args[1], "--help", 7) == 0)
    {
        printf("Use: %s\n\n", args[0]);
        printf("Description:\n");
        printf("\tSort args\n");
        printf("Options:\n");
        printf("\t--help\t display this help and exit\n");
        return 0;
    }

    auto heapPos = createNewSegment(HEAP_SIZE, (void *)HEAP_POS);
    setDefaultHeap(HeapInitialize(HEAP_SIZE, heapPos));

    merge_sort(nargs - 1, args + 1);

    for (int i = 1; i < nargs; i++)
        kprintf("%s\n", args[i]);

    return 0;
}

namespace
{
    void merge_sort(int n, char **args)
    {
        if (n <= 1)
            return;

        auto buffer = new char *[n];

        for (int i = 0; i < n; i++)
            buffer[i] = args[i];

        merge_sort(n / 2, buffer);
        merge_sort((n + 1) / 2, buffer + n / 2);

        int i = 0, j = 0, now = 0;

        while (i < n / 2 && j < (n + 1) / 2)
        {
            if (strncmp(buffer[i], buffer[n / 2 + j], HEAP_SIZE) <= 0)
                args[now++] = buffer[i++];
            else
                args[now++] = buffer[n / 2 + j++];
        }

        while (i < n / 2)
            args[now++] = buffer[i++];

        while (j < (n + 1) / 2)
            args[now++] = buffer[n / 2 + j++];

        delete[] buffer;
    }
} // namespace
