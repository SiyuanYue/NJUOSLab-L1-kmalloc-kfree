#include <common.h>
freenode_t *head;
freenode_t *head_page;
void *page_store;
#ifndef TEST
#define NOPE 1
#define YES 0
typedef int lock_t;
void lock(lock_t *locked)
{
	while (atomic_xchg(locked, NOPE) != 0)
	{
		; // spin there
	}
	assert(*locked == NOPE);
}
void unlock(lock_t *locked)
{
	atomic_xchg(locked, YES);
}
void lockinit(lock_t *locked)
{
	*locked = YES;
}
#else
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
typedef pthread_mutex_t lock_t;
void lock(lock_t *locked)
{
	int rc = pthread_mutex_lock(locked);
	assert(rc == 0);
}
void unlock(lock_t *locked)
{
	int rc = pthread_mutex_unlock(locked);
	assert(rc == 0);
}
void lockinit(lock_t *locked)
{
	pthread_mutex_init(locked, NULL);
}
#endif

lock_t heap_lock;
lock_t page_lock;

void *isAbleHold(void *start, size_t *size)
{
	void *add = start + sizeof(header_t);
	size_t powval = sizeof(header_t);
	while (powval < *size) // address is n times powval
	{
		powval *= 2;
	}
	*size = powval;
	uintptr_t n_add;
	freenode_t *node = (freenode_t *)start;
	if ((uintptr_t)add % powval == 0) // BUG!!!  FUCK!!! 这样即使当这个freenode size不够时，只要地址是整倍数也会分配！！！！
		n_add = (uintptr_t)add;
	else
		n_add = ((uintptr_t)add / powval + 1) * powval;
	if (n_add + *size <= (uintptr_t)start + node->size + sizeof(freenode_t))
	{
		return (void *)n_add;
	}
	else
		return NULL;
}

static void *kalloc(size_t size)
{

	if (size >= (100 << 20))
		return NULL;
	if (size <= sizeof(freenode_t)) // in x86 size>=16
		size = sizeof(freenode_t);
	if (size == 4096)
	{
		lock(&page_lock);
		freenode_t *h_page = head_page;
		if (h_page->size >= 4096)
		{
			h_page->size -= 4096;
			if (h_page->size == 0)
			{
				assert(h_page->magic == 234567);
				assert(h_page == head_page);
				head_page = head_page->next;
				memset(h_page, 0, 4096);
				unlock(&page_lock);
				return h_page;
			}
			else
			{
				assert(h_page->magic == 234567);
				assert(h_page == head_page);
				freenode_t *node = (void *)h_page + 4096;
				node->size = h_page->size;
				node->next = h_page->next;
				node->magic = 234567;
				head_page = node;
				memset(h_page, 0, 4096);
				unlock(&page_lock);
// #ifdef TEST
// 				printf(" alloc: %p---->%p  node :%p \n   ", (void *)h_page, (void *)h_page + 4096, (void *)node);
// 				fflush(stdout);
// #endif
				return h_page;
			}
		}
	}

// find the proper node od freelist
freenode_t *oldfreenode = NULL;

// freelist_t *prefreenode = NULL;
// get the lock ?  or first traverse then get lock?
// size_t proper_size = 0;// 最优匹配?

void *addr = NULL;
lock(&heap_lock);
#ifdef TEST
fflush(stdout);
#endif
// traverse
freenode_t *h = head;
freenode_t *prenode = NULL;

while (h)
{
	void *able_add = isAbleHold(h, &size);
	if (able_add != NULL) // can hold
	{
		assert(able_add != NULL);
		oldfreenode = h;
		addr = able_add;
		break;
		// if (proper_size < size)
		// {
		// 	proper_size = h->size;
		// 	addr = able_add;
		// 	oldfreenode = h;
		// 	prefreenode = prenode;
		// }
		// else if (h->size < proper_size)
		// {
		// 	proper_size = h->size;
		// 	addr = able_add;
		// 	oldfreenode = h;
		// 	prefreenode = prenode;
		// }
	}
	prenode = h;
	h = h->next; // traverse
}
// assert(addr != NULL); // or malloc failed
if (!addr)
{
	h = head;
	printf("\n -----freelist over: \n");
	return NULL;
}
assert(oldfreenode != NULL);
freenode_t *freenode = addr + size; // addr is n times 16 ,size too. So freenode must n times 16.
// #ifdef TEST
// printf("freenode  %p : %p---->%p   ",(void *)oldfreenode,(void *)oldfreenode+sizeof(freelist_t),(void *)oldfreenode+sizeof(freelist_t)+oldfreenode->size);
// fflush(stdout);
// #endif
if ((uintptr_t)freenode < (uintptr_t)oldfreenode + oldfreenode->size) // insert new freenode!
{
	freenode->next = oldfreenode->next;
	freenode->size = (uintptr_t)oldfreenode + oldfreenode->size - (uintptr_t)freenode; //+sizeof(freelist_t) - sizeof(freelist_t);
	if (prenode == NULL)
	{
		assert(oldfreenode == head);
		head = freenode;
	}
	else
	{
		assert(prenode->next == oldfreenode);
		prenode->next = freenode;
	}
}
else
{ // BUG FOGOT THIS SITUATION
	if (prenode)
	{
		assert(prenode->next == oldfreenode);
		prenode->next = oldfreenode->next;
	}
	else
	{
		assert(oldfreenode == head);
		head = head->next;
	}
}
unlock(&heap_lock);
memset(addr - sizeof(freenode_t), 0, sizeof(freenode_t));
header_t *header = (header_t *)(addr - sizeof(header_t));
header->size = size; // need to be 2^i ?
header->magic = 1234567;
if ((void *)oldfreenode != (void *)header) // TODO 需要对齐 NEW
{
	header->start = oldfreenode;
}
else
{
	header->start = NULL;
}
memset(addr, 0, size);

// #ifdef TEST
// 	printf("alloc:  %lx --->%lx  \n ",(uintptr_t)addr,(uintptr_t)(addr+size));
// 	fflush(stdout);
// #endif
return addr;
}

static void kfree(void *ptr)
{
	assert(ptr != NULL);
	if (ptr >= page_store)
	{
		lock(&page_lock);
		memset(ptr, 0, 4096);
		freenode_t *node = ptr;
		node->size = 4096;
		node->magic = 234567;
		node->next = head_page;
		head_page = node;
		unlock(&page_lock);
		return;
	}
	// #ifdef TEST
	// 	printf("  %p  \n", ptr);
	// 	fflush(stdout);
	// #endif
	header_t *header = (header_t *)(ptr - sizeof(header_t));
	assert(header->magic == 1234567);
	size_t size = 0;
	// clear
	if (header->start != NULL)
	{
		size = header->size + (void *)header - header->start;
		ptr = header->start + sizeof(header_t);
	}
	else
	{
		size = header->size;
	}
	memset(ptr - sizeof(header_t), 0, size + sizeof(header_t)); // clear

	lock(&heap_lock);
	// can release ?
	freenode_t *after = NULL;
	freenode_t *h = head;
	while (h)
	{
		if ((uintptr_t)h > (uintptr_t)header)
		{
			if (after == NULL)
				after = h;
			else if (h < after)
				after = h;
		}
		h = h->next;
	}

	// insert a freelist node
	freenode_t *freenode = ptr - sizeof(header_t);
	freenode->size = size;
	freenode->next = head;
	head = freenode;

	// Merge if could
	if (ptr + size == after)
	{
		// delete after
		h = head;
		while (h)
		{
			if (h->next == after)
				break;
			h = h->next;
		}
		// h->size=h->size+after->size+sizeof(freelist_t);  // modify freenode size not h!!!!//BUG
		freenode->size = freenode->size + after->size + sizeof(freenode_t);
		size = freenode->size;
		h->next = after->next;
	}
	unlock(&heap_lock);
	// #ifdef TEST
	// 	fflush(stdout);
	// 	printf("free: %lx - %lx \n",(uintptr_t)ptr,(uintptr_t)ptr+size);
	// 	fflush(stdout);
	// #endif
	return;
}

#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init()
{
	uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);

	printf("Got %d MiB heap: [%x, %x)\n", pmsize >> 20, heap.start, heap.end);
	printf("header size: %d\n", sizeof(header_t));
	printf("node size: %d\n", sizeof(freenode_t));
	head = heap.start;
	head_page = (void *)((uintptr_t)heap.end - 4096 * 1024);
	page_store = head_page;
	assert(head != NULL);
	head->size = (void *)head_page - heap.start - sizeof(freenode_t);
	head->next = NULL;
	head_page->next = NULL;
	head_page->magic = 234567;
	head_page->size = (uintptr_t)heap.end - (uintptr_t)head_page;
	assert(((uintptr_t)head_page) % 4096 == 0);
	//LOCK INIT
	lockinit(&heap_lock);
	lockinit(&page_lock);
	printf("strart   head_page: %p \n", page_store);
}
#else
// 测试代码的 pmm_init ()
#define HEAP_SIZE 16 * 1024 * 1024
typedef struct
{
	void *start, *end;
} area;
area heap;
static void pmm_init()
{
	char *ptr = malloc(HEAP_SIZE);
	heap.start = ptr;
	heap.end = ptr + HEAP_SIZE;
	heap.end = (void *)((((uintptr_t)heap.end / 4096) - 1) * 4096);
	printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
	head = heap.start;
	head_page = (void *)((uintptr_t)heap.end - 4096 * 1024);
	page_store = head_page;
	assert(head != NULL);
	head->size = (void *)head_page - heap.start - sizeof(freenode_t);
	head->next = NULL;
	head_page->next = NULL;
	head_page->magic = 234567;
	head_page->size = (uintptr_t)heap.end - (uintptr_t)head_page;
	assert(((uintptr_t)head_page) % 4096 == 0);
	//LOCK INIT
	lockinit(&heap_lock);
	lockinit(&page_lock);
	printf("strart   head_page: %p \n", page_store);
}
#endif

MODULE_DEF(pmm) = {
	.init = pmm_init,
	.alloc = kalloc,
	.free = kfree,
};
