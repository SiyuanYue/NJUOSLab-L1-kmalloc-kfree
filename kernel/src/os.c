#include <common.h>
static void os_init()
{
	pmm->init();
}

static void os_run()
{
	for (const char *s = "Hello World from CPU #*\n"; *s; s++)
	{
#ifndef TEST
		putch(*s == '*' ? '0' + cpu_current() : *s);
#else
		static size_t cpucnt = 0;
		putchar(*s == '*' ? '0' + cpucnt++ : *s);
#endif
	}
	void *add1 = pmm->alloc(1020);
	if (add1 == NULL)
	{
		printf("add is NULL");
		halt(1);
	}
	else
		printf("add1: %x\n", add1);

	int *add1_int=(int *)add1;
	*add1_int=9876;
	*(add1_int+(1020-4)/4)=114514;
	void *add2 = pmm->alloc(20);
	if (add2 == NULL)
	{
		printf("add2 is NULL");
		halt(1);
	}
	else
		printf("add2: %x\n", add2);
	
	void *add3 = pmm->alloc(512);
	if (add3 == NULL)
	{
		printf("add3 is NULL");
		halt(1);
	}
	else
		printf("add3: %x\n", add3);
	int *add3_int=(int *)add3;
	*add3_int=543210;
	*(add3_int+(512-4)/4)=114514;
	pmm->free(add2);
	assert(*add1_int==9876);
	assert(*(add1_int+(1020-4)/4)==114514);
	assert(*add3_int==543210);
	assert(*(add3_int+(512-4)/4)==114514);
	pmm->free(add1);
	assert(*add3_int==543210);
	assert(*(add3_int+(512-4)/4)==114514);
	pmm->free(add3);
	while(true);
}

MODULE_DEF(os) = {
	.init = os_init,
	.run = os_run,
};
