#include <common.h>

freenode_t *head;
// freenode_t *head_huge;
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
#include <sys/types.h>
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
#include <sys/syscall.h>
#include <unistd.h>
pid_t gettid()
{
	return syscall(SYS_gettid);
}
#endif
typedef struct pagelist_t //每个CPU或线程各有一个的Pagelist，串起来的页列表
{
	pageheader_t *pagehead;//这个CPU的所属page链表
	lock_t lock;
} pagelist_t;
typedef struct thread_alloc_list_t
{
	lock_t th_lock; //大锁,分配页时候用到的锁
	int thread_count;
	unsigned threadpage_index[THREADMAX];
	pagelist_t thread_page[THREADMAX];
	int magic;
} thread_alloc_list_t;

lock_t heap_lock;

#ifndef TEST
pagelist_t cpupagelist[16];
#else
thread_alloc_list_t th_alloclist;
#endif
void allockpage(pageheader_t **pagehead) // page = 64KiB
{
	freenode_t *oldfreenode = NULL;
	lock(&heap_lock);
	// traverse
	freenode_t *h = head;
	freenode_t *prenode = NULL;
	while (h)
	{
		if (h->size >= 64 * 1024) // can hold
		{
			oldfreenode = h;
			break;
		}
		prenode = h;
		h = h->next; // traverse
	}
	if (oldfreenode)
	{
		freenode_t *freenode = (void *)oldfreenode + 64 * 1024;
		if (oldfreenode->size == 64 * 1024)
		{
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
			*pagehead = (void *)oldfreenode;
			memset(oldfreenode, 0, sizeof(freenode_t));
			unlock(&heap_lock);
			return;
		}
		else
		{
			assert(oldfreenode->size > 64 * 1024);
			freenode->next = oldfreenode->next;
			freenode->size = oldfreenode->size - 64 * 1024;
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
			*pagehead = (void *)oldfreenode;
			memset(oldfreenode, 0, sizeof(freenode_t));
			unlock(&heap_lock);
			return;
		}
	}
	else
	{
		unlock(&heap_lock);
		*pagehead = NULL;
		return;
	}
}
static void *kallocHugeMemory(size_t size)
{
	lock(&heap_lock);
	freenode_t *h = head;
	freenode_t *highnode = NULL;
	while (h)
	{
		if (h->size >= size) // can hold
		{
			if (highnode == NULL)
				highnode = h;
			else if (h > highnode)
				highnode = h;
		}
		h = h->next; // traverse
	}
	if (highnode)
	{
		void *ret = (void *)highnode + highnode->size - size; // BUG 这里没有转成void *!!!
		header_t *header = ret - sizeof(header_t);
		highnode->size = (uintptr_t)header - (uintptr_t)highnode; // BUG 减反了
		unlock(&heap_lock);
		header->size = size;
		header->magic = 7654321;
		return ret;
	}
	else
	{
		unlock(&heap_lock);
		return NULL;
	}
}
static void *kalloc(size_t size)
{
	if (size <= 32)
		size = 32;
	else if (size > 16 * 1024) // when size >16KB  slowpath
	{
		if(size >16*1024*1024)
			return NULL;
		size_t powval = 32;
		while (powval < size) // address is n times powval
		{
			powval *= 2;
		}
		size = powval;
		void *addr = kallocHugeMemory(size);
		return addr;
	}
	else
	{
		size_t powval = 32;
		while (powval < size) // address is n times powval
		{
			powval *= 2;
		}
		size = powval;
	}
#ifndef TEST
	int cpuno = cpu_current();
	assert(cpuno <= cpu_count());
	if (cpupagelist[cpuno].pagehead == NULL) // cpu first alloc
	{
		allockpage(&cpupagelist[cpuno].pagehead); // lock and alloc a 64Kib page
		lock(&cpupagelist[cpuno].lock);
		assert(cpupagelist[cpuno].pagehead != NULL);
		cpupagelist[cpuno].pagehead->next = NULL;
		cpupagelist[cpuno].pagehead->size = size;
		cpupagelist[cpuno].pagehead->pagefreehead_index = 1;
		void *pagefreend = (void *)cpupagelist[cpuno].pagehead + size; // index=1 init first freenode
		pagefreenode_t *pageFreend = (pagefreenode_t *)pagefreend;
		pageFreend->magic = 1234567;
		pageFreend->next = NULL;
		pageFreend->size = 64 * 1024 - size;
		unlock(&cpupagelist[cpuno].lock);
	}
	pageheader_t *ph = cpupagelist[cpuno].pagehead;
	while (ph)
	{
		if (ph->size == size && ph->pagefreehead_index > 0)//ph->pagefreehead_index == 0时这页已满
 		{
			break;
		}
		ph = ph->next;
	}
	if (ph) // fastpath
	{
		assert(ph->size == size);
		pagefreenode_t *fd = (void *)ph + ph->pagefreehead_index * size;
		if (fd->size >= size) // fastpath
		{
			fd->size -= size;
			if (fd->size == 0) // free过的一块 or last kuai
			{
				assert(fd->magic == 1234567);
				if (fd->next == NULL) //after this alloc, this page is full
				{
					ph->pagefreehead_index = 0;
				}
				else
				{
					ph->pagefreehead_index = ((uintptr_t)fd->next - (uintptr_t)ph) / size;
					assert((uintptr_t)ph + ph->pagefreehead_index * size == (uintptr_t)fd->next);
				}
			}
			else
			{
				assert(ph->size == size);
				pagefreenode_t *newnode = (void *)fd + size;
				newnode->magic = 1234567;
				newnode->next = fd->next;
				newnode->size = fd->size; // 上面减过了
				ph->pagefreehead_index++;
				assert((uintptr_t)ph + ph->pagefreehead_index * size == (uintptr_t)newnode);
				assert(ph->pagefreehead_index < 64 * 1024 / size);
			}
			memset(fd, 0, size);
			return fd;
		}
		else
		{
			panic_on(true, "not do");
			return fd;
		}
	}
	else //这种大小的页还没有 slowpath
	{
		pageheader_t *pagehead = cpupagelist[cpuno].pagehead;
		while (pagehead)
		{
			if (pagehead->size == 0 && pagehead->pagefreehead_index > 0)
			{
				pagehead->size = size;
				break;
			}
			pagehead = pagehead->next;
		}
		if (pagehead == NULL)
		{
			allockpage(&pagehead);
			if (pagehead) // insert page
			{
				lock(&cpupagelist[cpuno].lock);
				pagehead->next = cpupagelist[cpuno].pagehead;
				pagehead->size = size;
				pagehead->pagefreehead_index = 1;
				cpupagelist[cpuno].pagehead = pagehead;
				unlock(&cpupagelist[cpuno].lock);
			}
			else
			{
				return NULL;
			}
		}
		void *ret = (void *)pagehead + pagehead->pagefreehead_index * size;
		pagefreenode_t *newnode = ret + size;
		newnode->magic = 1234567;
		newnode->next = NULL;
		pagehead->pagefreehead_index ++;
		newnode->size = 64 * 1024 - pagehead->pagefreehead_index * size;
		assert((uintptr_t)pagehead + pagehead->pagefreehead_index * size == (uintptr_t)newnode);
		memset(ret, 0, size);
		return ret;
	}
#endif
	return NULL;
}

static void kfree(void *ptr)
{
#ifndef TEST
	assert(ptr != NULL);
	lock(&cpupagelist[cpu_current()].lock);
	pageheader_t *ph = cpupagelist[cpu_current()].pagehead;
	while (ph)
	{
		if ((uintptr_t)ph < (uintptr_t)ptr && (uintptr_t)ptr < (uintptr_t)ph + 64 * 1024)
			break;
		ph = ph->next;
	}
	if (ph)
	{
		memset(ptr, 0, ph->size);
		pagefreenode_t *after = NULL;
		pagefreenode_t *before = NULL;
		pagefreenode_t *h = (void *)ph + ph->pagefreehead_index * ph->size;
		while (h)
		{
			if ((uintptr_t)h > (uintptr_t)ptr)
			{
				if (after == NULL)
					after = h;
				else if (h < after)
					after = h;
			}
			if ((uintptr_t)h < (uintptr_t)ptr)
			{
				if (before == NULL)
					before = h;
				else if (h > before)
					before = h;
			}
			h = h->next;
		}
		if (before && (uintptr_t)before + before->size == (uintptr_t)ptr)
		{ // before can merge
			before->size = before->size + ph->size;
			if (ptr + ph->size == (void *)after)
			{
				before->size += after->size;
				h = (void *)ph + ph->pagefreehead_index * ph->size;
				while (h)
				{
					if (h->next == after)
						break;
					h = h->next;
				}
				if (h)
					h->next = after->next;
				else
					ph->pagefreehead_index = ((uintptr_t)before - (uintptr_t)ph) / ph->size;
			}
			if (before->size == 64 * 1024 - ph->size)
			{
				ph->size = 0;
			}
		}
		else if (ptr + ph->size == (void *)after)
		{ // after can merge
			pagefreenode_t *pf = (pagefreenode_t *)ptr;
			pf->magic = 1234567;
			pf->size = ph->size + after->size;
			pf->next = (void *)ph + ph->pagefreehead_index * ph->size;
			ph->pagefreehead_index = ((uintptr_t)pf - (uintptr_t)ph) / ph->size;
			assert(ph->pagefreehead_index > 0);
			h = (void *)ph + ph->pagefreehead_index * ph->size;
			while (h) // delete after
			{
				if (h->next == after)
					break;
				h = h->next;
			}
			h->next = after->next;
			if (pf->size == 64 * 1024 - ph->size)
			{
				ph->size = 0;
			}
		}
		else
		{
			pagefreenode_t *pf = (pagefreenode_t *)ptr;
			pf->magic = 1234567;
			pf->size = ph->size;
			pagefreenode_t *oldpfhead = (void *)ph + ph->pagefreehead_index * ph->size;
			pf->next = oldpfhead;
			ph->pagefreehead_index = ((uintptr_t)pf - (uintptr_t)ph) / ph->size;
			assert(ph->pagefreehead_index > 0);
		}
		unlock(&cpupagelist[cpu_current()].lock);
		return;
	}
	else //分配的大内存how释放
	{
		unlock(&cpupagelist[cpu_current()].lock);
		lock(&heap_lock);
		header_t *header = ptr - sizeof(header_t);
		freenode_t *after = NULL;
		freenode_t *before = NULL;
		freenode_t *h = head;
		// printf("%x  %x\n", ptr, header);
		assert(header->magic == 7654321);
		size_t size = header->size;
		memset(ptr - sizeof(header_t), 0, header->size + sizeof(header_t));
		freenode_t *freenode = (void *)header;
		while (h)
		{
			if ((uintptr_t)h > (uintptr_t)header)
			{
				if (after == NULL)
					after = h;
				else if (h < after)
					after = h;
			}
			if ((uintptr_t)h < (uintptr_t)header)
			{
				if (before == NULL)
					before = h;
				else if (h > before)
					before = h;
			}
			h = h->next;
		}
		if (before && (uintptr_t)before + before->size == (uintptr_t)header)
		{ // before can merge
			before->size = before->size + size + sizeof(header_t);
			if (ptr + size == (void *)after)
			{
				before->size += after->size;
				h = head;
				while (h)
				{
					if (h->next == after)
						break;
					h = h->next;
				}
				if (h)
					h->next = after->next;
				else
					head = head->next;
			}
		}
		else if (ptr + size == (void *)after)
		{ // after can merge
			freenode->size = size + after->size;
			freenode->next = head;
			head = freenode;
			h = freenode; // <=> h=head;
			while (h)	  // delete after
			{
				if (h->next == after)
					break;
				h = h->next;
			}
			h->next = after->next;
		}
		else
		{
			// printf("%x %x %x\n",head,heap_end,freenode);
			freenode->size = size;
			freenode->next = head;
			head = freenode;
		}
		unlock(&heap_lock);
		return;
	}
#endif
}

#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init()
{
	uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
	printf("Got %d MiB heap: [%x, %x)\n", pmsize >> 20, heap.start, heap.end);
	head = heap.start;
	assert(head != NULL);
	head->size = heap.end - heap.start;
	head->next = NULL;
	lockinit(&heap_lock);
	for (size_t i = 0; i < cpu_count(); i++)
	{
		cpupagelist[i].pagehead = NULL;
		lockinit(&cpupagelist[i].lock);
	}
	lockinit(&heap_lock);
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
	// LOCK INIT
	lockinit(&heap_lock);
	lockinit(&page_lock);
	th_alloclist.thread_count = 0;

	printf("sizeof(thread_alloc_LIST) %d   tid:%d\n", (int)sizeof(thread_alloc_list_t), gettid());
}
#endif

MODULE_DEF(pmm) = {
	.init = pmm_init,
	.alloc = kalloc,
	.free = kfree,
};
