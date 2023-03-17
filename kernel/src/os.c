#include <common.h>
static void os_init()
{
	pmm->init();
}
// #define TEST
#ifdef TEST
static void os_run_test3()//大内存
{
	void *add;
	add=pmm->alloc(64*4096);
	if (add== NULL)
	{
		printf("add is NULL");
		halt(1);
	}
	else
		printf("add: %x\n", add);
	for (size_t i = 0; i <100000000; i++)
	{
		;
	}
	pmm->free(add);
}
static void os_run_test1()
{
	void *add1[100];
	void *add2[100];
	for (size_t i = 0; i < 100; i++)
	{
		add1[i] = pmm->alloc(24);
		if (add1[i] == NULL)
		{
			printf("add is NULL");
			halt(1);
		}
		else
			printf("add1[%d]: %x\n",i, add1[i]);
		int *add1_int=(int *)add1[i];
		*add1_int=9876;
		*(add1_int+(24-4)/4)=114514;
		add2[i] = pmm->alloc(64);
		if (add2[i] == NULL)
		{
			printf("add2[%d] is NULL",i);
			halt(1);
		}
		else
			printf("add2[%d]: %x\n",i, add2[i]);
	}
	for (size_t i = 0; i < 100; i++)
	{
		int *add1_int=(int *)add1[i];
		assert(*add1_int=9876);
		assert(*(add1_int+(24-4)/4)=114514);
		pmm->free(add1[i]);
		pmm->free(add2[i]);
	}
}
static void os_run_test2()
{
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
	void *add2 = pmm->alloc(510);
	if (add2 == NULL)
	{
		printf("add3 is NULL");
		halt(1);
	}
	else
		printf("add2: %x\n", add2);

	void *add3 = pmm->alloc(800);
	if (add3 == NULL)
	{
		printf("add3 is NULL");
		halt(1);
	}
	else
		printf("add3: %x\n", add3);
	int *add3_int=(int *)add3;
	*add3_int=543210;
	*(add3_int+(800-4)/4)=114514;
	pmm->free(add2);
	assert(*add1_int==9876);
	assert(*(add1_int+(1020-4)/4)==114514);
	assert(*add3_int==543210);
	assert(*(add3_int+(800-4)/4)==114514);
	pmm->free(add1);
	assert(*add3_int==543210);
	assert(*(add3_int+(800-4)/4)==114514);
	pmm->free(add3);
}
#endif
static void os_run()
{
	for (const char *s = "Hello World from CPU #*\n"; *s; s++)
	{
		putch(*s == '*' ? '0' + cpu_current() : *s);
	}
#ifdef TEST
	os_run_test1();
	os_run_test2();
	os_run_test3();
#endif
	printf("%x\n",pmm->alloc(5368));
	printf("%x\n",pmm->alloc(4096));
	printf("%x\n",pmm->alloc(5368));
	printf("%x\n",pmm->alloc(4));

	while(true);
}

MODULE_DEF(os) = {
	.init = os_init,
	.run = os_run,
};
