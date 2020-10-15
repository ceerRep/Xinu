/*  main.c  - main */

#include <xinu.h>

void puta(void);
void putb(void);

process main(void)
{
	resume(create(puta, 8192, 20, "🦄", 0));
	resume(create(putb, 8192, 20, "🐴", 0));
	return OK;
}

void puta(void)
{
	while (1)
		kprintf("🦄");
}

void putb(void)
{
	while (1)
		kprintf("🐴");
}

