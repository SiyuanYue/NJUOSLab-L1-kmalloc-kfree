#include "common.h"
#include "thread.h"
#include "assert.h"
static void entry0(int tid)
{
	void *add1 = pmm->alloc(1020);
	if (add1 == NULL)
	{
		puts("add is NULL");
		exit(1);
	}
	else
		printf("add1: %p\n", add1);

	int *add1_int=(int *)add1;
	*add1_int=9876;
	*(add1_int+(1020-4)/4)=114514;
	void *add2 = pmm->alloc(20);
	if (add2 == NULL)
	{
		puts("add3 is NULL");
		exit(1);
	}
	else
		printf("add2: %p\n", add2);
	
	void *add3 = pmm->alloc(512);
	if (add3 == NULL)
	{
		puts("add3 is NULL");
		exit(1);
	}
	else
		printf("add3: %p\n", add3);
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
}
static void entry1(int tid)//page test
{
	void *add = pmm->alloc(4096);
	if (add == NULL)
	{
		puts("add is NULL");
		exit(1);
	}
	else
		printf("add: %p\n", add);
	char *add_char=(char *)add;
	*add_char='a';
	*(add_char+4095)='b';
	void *add1 = pmm->alloc(4096);
	if (add1 == NULL)
	{
		puts("add1 is NULL");
		exit(1);
	}
	else
		printf("add1: %p\n", add1);

	// for(int i=0;i<=5;++i)
	// {
	// 	void *add = pmm->alloc(4096);
	// 	if (add == NULL)
	// 	{
	// 		puts("add is NULL");
	// 		exit(1);
	// 	}
	// 	else
	// 		printf("add: %p\n", add);
	// }

	char *add_char1=(char *)add1;
	*add_char1='c';
	*(add_char1+4095)='d';
	assert(*add_char=='a');
	assert(*(add_char+4095)=='b');
	pmm->free(add);
	assert(*add_char1=='c');
	assert(*(add_char1+4095)=='d');
	pmm->free(add1);
}
static void entry2(int tid) //混合的内存申请
{
	void *add = pmm->alloc(4096);
	if (add == NULL)
	{
		puts("add is NULL");
		exit(1);
	}
	else
		printf("add: %p\n", add);
	char *add_char=(char *)add;
	*add_char='a';
	*(add_char+4095)='b';
	void *add1 = pmm->alloc(1024);
	if (add1 == NULL)
	{
		puts("add1 is NULL");
		exit(1);
	}
	else
		printf("add1: %p\n", add1);
	char *add_char1=(char *)add1;
	*add_char1='c';
	*(add_char1+1023)='d';
	assert(*add_char=='a');
	assert(*(add_char+4095)=='b');
	pmm->free(add);
	assert(*add_char1=='c');
	assert(*(add_char1+1023)=='d');
	void *add2 = pmm->alloc(45);
	if (add2 == NULL)
	{
		puts("add2 is NULL");
		exit(1);
	}
	else
		printf("add2: %p\n", add2);
	char *add_char2=(char *)add2;
	*add_char2='e';
	*(add_char2+44)='f';
	pmm->free(add1);
	assert(*add_char2=='e');
	assert(*(add_char2+44)=='f');
	add = pmm->alloc(4096);
	if (add == NULL)
	{
		puts("add is NULL");
		exit(1);
	}
	else
		printf("add: %p\n", add);
	add_char=(char *)add;
	*add_char='g';
	*(add_char+4095)='h';
	pmm->free(add2);
	assert(*add_char=='g');
	assert(*(add_char+4095)=='h');
	pmm->free(add);
}
static void goodbye() { printf("End.\n"); }
void do_test_0()
{
	pmm->init();
	for (int i = 0; i < 10; i++)
	{
		create(entry0);
	}
	join(goodbye);
}
void do_test_1()
{
	pmm->init();
	for (int i = 0; i < 10; i++)
	{
		create(entry1);
	}
	join(goodbye);
}
void do_test_2()
{
	pmm->init();
	for (int i = 0; i < 10; i++)
	{
		create(entry2);
	}
	join(goodbye);

}
int main(int argc, char *argv[]) {
  if (argc < 2) exit(1);
  switch(atoi(argv[1])) {
    case 0: do_test_0(); break;
    case 1: do_test_1(); break;
	case 2: do_test_2(); break;
	default: break;
  }
  return 0;
}