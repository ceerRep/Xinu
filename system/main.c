/*  main.c  - main */

#include <xinu.h>

int proccnt[NPROC] = {0};

_Noreturn process Worker(void)
{
    while (1)
    {
        for (register uint32 i = 0; i < 100000; i++)
            proccnt[currpid]++;
		sleepms(2);
    }
}

process main(void)
{

    /* Run the Xinu shell */

    // recvclr();
    // resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

    // /* Wait for shell to exit and recreate it */

    // while (TRUE) {
    // 	receive();
    // 	sleepms(200);
    // 	kprintf("\n\nMain process recreating shell\n\n");
    // 	resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
    // }

    resume(create(Worker, 4096, 10, "A1", 0));
    resume(create(Worker, 4096, 10, "A2", 0));
    resume(create(Worker, 4096, 20, "B", 0));
    resume(create(Worker, 4096, 30, "C", 0));
    return OK;
}
