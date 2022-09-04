# NJUOSLAB L1 Kmalloc/Kfree

### 实验手册请看：[L1: 物理内存管理 (pmm) (jyywiki.cn)](http://jyywiki.cn/OS/2022/labs/L1)

> 写在前面：
> 这次实验花费时间很长，是上大学以来做的最困难的实验。。。
> 代码量大，正确性的保障非常艰难，很容易犯一些难以察觉的小错误。
> *并且与并发编程相关联，要求在性能与安全之间做出度量，不过好在jyy老师讲解slab实现方式，最后采取slab实现更高并发度良好性能的分配器。*

---

L1的代码位于 ` ./kenel/`文件夹下。

**如果要启动多个处理器，可以为 `make run` 传递 `smp` 环境变量，例如 `smp=2`代表启动 2 个处理器；`smp=4` 代表启动 4 个处理器。**

~~使用 `make test`可以在本地生成一个 `test`的可执行文件，不经过qemu,也不调用裸机的klib，直接链接libc ,在本地测试/调试代码,节省了编译、运行、调试……整个开发流程的 overhead。当然这需要在编程时频繁使用宏写出“两份代码”。~~(master分支没有编写能在本地运行来方便测试与调试的代码,另一个分支可以)



---

# 1.实验描述与要求：

### 1. 1实现多处理器安全的内存分配和回收

1. 在 AbstractMachine 启动后，`[heap.start, heap.end)` 会给出一段可用的物理内存 (堆区)。你需要在此基础上实现允许多个处理器**并发**地申请或释放内存的分配器
2. **对于大小为 s 的内存分配请求，返回的内存地址必须对齐到 2^i**，
3. **允许多处理器并行地调用 kalloc/kfree**：
   - 不同的处理器可能同时执行 kalloc 分配大小不同的内存；
   - 不同的处理器可能同时执行 kfree 释放内存；
   - 在 kalloc/kfree 实现正确的前提下，尽可能使不同处理器上的内存分配能够并行。如果你代码的性能过低 (例如用全局的锁保护空闲链表，并用遍历链表的方式寻找可用的内存)，可能会导致 hard test failure。

### 1.2. 性能优化 (Hard Tests)

多处理器上的内存分配和释放带来额外的挑战：

1. 我们希望保证分配的正确性。
2. 我们希望保证分配在多处理器上的性能和伸缩性

### 1.3. 实现 AbstractMachine 中 klib 中缺失的函数

不实现 `printf、sprintf、memset、strlen`等基础函数，会寸步难行的，毕竟
**printf是最好的调试器（bushi）🤭**

---

# 2. 第一次尝试

采取传统的教科书上的空闲链表和头节点管理整个堆区的内存再配一把大锁，很安全但不高效，尤其是多处理器分配小块内存。
我实现不想写了。。。写不动了。。。
已经实现完了这一套，但看JYY（[C 标准库的实现 (系统调用的封装；内存空间管理) [南京大学2022操作系统-P14]_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV17F411s7e9/?spm_id_from=333.788&vd_source=33d3156975c92d1beb9e11e8b218f8b0)）时， 看他说了这种方法的严重弊端和细致介绍了Slab，**我就整个重写了**，实现了类似 slab方式的内存分配。

但毕竟已经实现了这一套传统教科书的方式（毕竟代码写都写了/(ㄒoㄒ)/~~
会上传在分支捏。）

---

# 3. 第二次尝试

### 3.1 性能考量

聆听了jyy那节课（见2）后感受深刻，决定更改原本粗糙野生的一把大锁的方案，借鉴slab的思想，极大提高分配器的并行度。

正如课上所讲我们在思考实际性能进行性能优化前，就必须考虑实际 `workload`。

 Slab的思想有些像存储器层次结构中的cache（缓存）。
 实现我们考虑的实际的 `workload` 。内存分配的实际workload是这样的：

- 越小的对象创建/分配越频繁
  -   字符串、临时对象等 (几十到几百字节)；生存周期可长可短
- 较为频繁地分配中等大小的对象
  - 较大的数组、复杂的对象；更长的生存周期
- 低频率的大对象
  - 巨大的容器、分配器；很长的生存周期

我们需要极大提高并行度，因此，像传统操作系统书一样简单的使用空闲链表并不符合实际场景要求，在连续分配小内存这种频繁场景会面临一把大锁卡住拥堵。

因此考虑 fastpath 和 slowpath 应该实现两套系统，在小内存频繁分配场景我们应当实现更快速度（fastpath）即不需要上🔒或者能顺利拿到🔒，不会出现堵塞。在分配大内存这样的低频率场景采取 slowpath，要求获取一把大锁，保证安全性。

为使使所有 CPU 都能并行地申请内存，我们为每个 cpu/thread 分配一些专属于他们的内存，在这些地方上分配时不需要发生竞争和拥堵，acquire lock 几乎总是成功。当我们的内部内存不够或者分配大内存这样的 slowpath再去获得大锁去栈空间分配。

其实就是： Segregated List (Slab)            ----摘自JYY PPT
分配:

- 每个 slab 里的每个对象都一样大（便于管理）
  - 每个线程拥有每个对象大小（2^i）的 slab
  - fast path → 立即在线程本地分配完成
  - slow path → 分配一个page（64KiB）或大内存分配
- 两种实现
  - 全局大链表 v.s. List sharding (per-page 小链表)
    回收
- 整个page直接归还到 slab 中
  - 注意这可能是另一个线程持有的 slab，需要 per-slab 锁 (小心数据竞争)

## 3.2 实现

1. 由于这个 abstarct-machine 提供了关于多处理器的 api：

```
int cpu_current();     返回当前执行流的 CPU 编号
int cpu_count();   返回系统中处理器的个数，在整个 AbstractMachine 执行期间数值不会变化。
```

因此我选取为每个 cpu分配一个 `pagelist_t`  ,  假定最多16个CPU，`pagelist_t cpupagelist[16];`位于全局变量（整个进程中所有处理器共享的内存）。

```C
typedef struct pagelist_t //每个CPU或线程各有一个的Pagelist，串起来的页列表
{
	pageheader_t *pagehead;//这个CPU的所属page链表
	lock_t lock;
} pagelist_t;
```

在每个 CPU在自己内部的缓存中分配或释放时，只需获得对应pagelist_t里的小锁，不会与其他 CPU竞争（fastpath）： `lock(&cpupagelist[cpucurrent()].lock);

2. cpupagelist[cpucurrent()] 中的 pagehead 是指这个 CPU 的所属 page链表。

```C
typedef struct pageheader_t{   //每个页的头结点包含信息
	struct pageheader_t *next;
	int size;  //表明该页分配的每块内存大小
	unsigned pagefreehead_index; //页中分配内存块的空闲链表起点
}pageheader_t;
```

其中 `unsigned pagefreehead_index;` 是页中分配内存块的空闲链表起点，从该头节点起乘size就是 `pagefreenode`页中分配内存块的空闲链表入口指针。

```C
typedef struct pagefreenode_t{
	struct pagefreenode_t *next;
	int size;  标记当前空闲链表节点所掌管的区域大小
	int magic;
}pagefreenode_t;
```

3. 那我们如何在整个堆空间中为我们每个cpu 或者线程分配固定大小的 page（64KiB）？
   答案就是第一次尝试中的传统的空闲链表。
   在获得整个堆区的大锁后，通过空闲链表分配页，空闲链表存储空闲内存信息。
   不需要保存分配页的头部节点，因为我们知道分配页的大小是64KiB。而且这样能保证地址对齐。
   分配页的时机： 这个cpu/线程第一次 alloc，当前所需内存块大小（对齐到2^i）的 page没有。
4. 这带来一个新问题如何知道我们要 free的内存是一个我们分配的页？

```C
	assert(ptr != NULL);
	lock(&cpupagelist[cpu_current()].lock);
	pageheader_t *ph = cpupagelist[cpu_current()].pagehead;
	while (ph)
	{
		if ((uintptr_t)ph < (uintptr_t)ptr && (uintptr_t)ptr < (uintptr_t)ph + 64 * 1024)
			break;
		ph = ph->next;
	}
```

    其实很简单就是在当前所属的page链表中搜索是否有页与这段内存相交。
        在每个的cpupagelist存储的page链表中搜索，是否有包含传递进来内存地址的page，然后释放固定大小。

5. 如何分配大内存？
   就是采用传统方法使用空闲链表和头部结点的方式，这次我们不能事先知道内存的大小，所以需要头部节点告知我们释放时内存块多大。

   **但是我们不想分配的大内存与我们之前在整个堆区分配链表的位置相交，这样我们无法保持页表的对齐等方便的特性，因此采取在堆尾部分配大内存**。

   这样又会有一个疑问，如果大内存分配过多，达到接近堆顶的位置，会不会分配 page不正确？
   答案是不会，其实页的分配与释放只需要经过 freenode，因为我们能通过在 cpupagelist中索引能精确知道当前内存是不是个 page，是的话就可以释放了，分配时只要当前空闲 freenode的大小能够满足并能够对齐，自然可以分配。
6. page的释放？
   我采取的策略是内存的释放无论是page内还是整个堆区的内存，在释放时都会考虑与前一个空闲点、后一个空闲结点合并。如果页中内存块释放后出现 `64 * 1024 - ph->size(第一个size大小内存块会一直放页头部结点 pageheader_t)`。
   这时说明该页全空，当全空时，我们会将该page的 `unsigned pagefreehead_index; //页中分配内存块的空闲链表起点`置为0 ,这样留着给下一次该CPU需要分配页时直接使用该页，出现第二个冗余的空page我们再将整个page释放。
7. 这次lab深刻的直接体会，想要实现越精妙的算法就需要更多的试错和实打实的代码量,而且非常容易带来错误！！！。。。
