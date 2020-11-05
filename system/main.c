/*  main.c  - main */

#include <xinu.h>

#define NUM 10
#define MAX_FOOD_COUNT 20000

pid32 procs[NUM * 3];
int proctype[NPROC] = {0};
long long proccnt[NPROC] = {0};
long long proccntabs[NPROC] = {0};

sid32 food_need, food_rev;

extern long long time_used;

void _Noreturn C()
{
    while (1)
    {
        wait(food_rev);
        signal(food_need);
        proccnt[currpid]++;
        proccntabs[currpid]++;
    }
}

void _Noreturn P()
{
    while (1)
    {
        wait(food_need);
        signal(food_rev);
        proccnt[currpid]++;
        proccntabs[currpid]++;
    }
}

void _Noreturn S()
{
    while (1)
    {
        if (proctype[currpid] & 1) // P
        {
            wait(food_need);
            signal(food_rev);
            proccnt[currpid]++;
            proccntabs[currpid]++;
        }
        else // C
        {
            wait(food_rev);
            signal(food_need);
            proccnt[currpid]--;
            proccntabs[currpid]++;
        }
    }
}

process main(void)
{
    food_need = semcreate(3 * NUM / 2);
    food_rev = semcreate(3 * NUM / 2);

    for (int i = 0; i < NUM; i++)
    {
        procs[i] = create(C, 8192, 30, "C", 0);
        proctype[procs[i]] = 0b10;
    }

    for (int i = 0; i < NUM; i++)
    {
        procs[NUM + i] = create(P, 8192, 20, "P", 0);
        proctype[procs[NUM + i]] = 0b11;
    }

    for (int i = 0; i < NUM; i++)
    {
        procs[2 * NUM + i] = create(S, 8192, 20, "S", 0);
        proctype[procs[2 * NUM + i]] = 0b111;
    }

    intmask mask = disable();
    for (int i = 0; i < NUM * 3; i++)
        resume(procs[i]);
    restore(mask);

    kprintf("\x1b[s");

    for (int rnd = 0;; rnd++)
    {
        long long sum[] = {0, 0, 0, 0, 0, 0, 0, 0};
        long long sumabs[] = {0, 0, 0, 0, 0, 0, 0, 0};
        intmask mask = disable();

        // Print statistics
        kprintf("\x1b[u%d %d\n", rnd, (int)time_used);

        for (int i = 0; i < NPROC; i++)
            if (proctab[i].prstate != PR_FREE)
            {
                sum[proctype[i]] += proccnt[i];
                sumabs[proctype[i]] += proccntabs[i];
            }

        for (int i = 0; i < 8; i++)
            kprintf("[%d] %d %d\n", i, (int)sum[i], (int)sumabs[i]);
        kprintf("------------------\n");

        restore(mask);

        yield();
    }

    return OK;
}
