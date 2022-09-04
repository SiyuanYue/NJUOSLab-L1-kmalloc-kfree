#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#define THREADMAX 1024
typedef struct pageheader_t{   //每个页的头结点包含信息
	struct pageheader_t *next;
	int size;   ////表明该页分配的每块内存大小
	unsigned pagefreehead_index;  //页中分配内存块的空闲链表起点
}pageheader_t;

typedef struct pagefreenode_t{
	struct pagefreenode_t *next;
	int size;
	int magic;
}pagefreenode_t;


typedef struct header_t
{
	void *nothing;
	int size;
	int magic;    
}header_t;
typedef struct freenode_t
{
	struct freenode_t* next;
	int size;
	int magic;
}freenode_t;	


void * isAbleHold(void *start,size_t *size);