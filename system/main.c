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

process  main(void)
{
  pid32 pids[4];
  pids[0] = create(Worker, 8192, 10, "1", 1, 0);
  pids[1] = create(Worker, 8192, 10, "2", 1, 1);
  pids[2] = create(Worker, 8192, 20, "3", 1, 2);
  pids[3] = create(Worker, 8192, 30, "4", 1, 3);

  for (uint32 i = 0; i < sizeof(pids) / sizeof(pid32); i++)
    resume(pids[i]);

  kprintf("\x1b[s");

  while (1) {
    intmask mask = disable();

    // Print statistics
    kprintf("\x1b[u------------------\n");
    
	for(int i = 0; i < 16; i++)
		if (proctab[i].prstate != PR_FREE)
			kprintf("%s [%d] %d\n", proctab[i].prname, i, proccnt[i]);
    kprintf("------------------\n");

    restore(mask);

    yield();
  }

  return OK;
}
