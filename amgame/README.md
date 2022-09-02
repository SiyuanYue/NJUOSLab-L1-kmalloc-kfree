# NJUOS L0 报告

## L0  直接运行在硬件上的小游戏 (amgame)

实验手册请看： [L0: 直接运行在硬件上的小游戏 (amgame) (jyywiki.cn)](http://jyywiki.cn/OS/2022/labs/L0)

主要任务其实是充分**阅读 AbstractMachine 文档**，了解什么是 `AbstractMachine`（[AbstractMachine: 抽象计算机 (jyywiki.cn)](http://jyywiki.cn/AbstractMachine/)：一句话：AbstractMachine 解决的问题是 “能否在不理解硬件机制细节的前提下实现操作系统”。），何为裸机环境 `Bare-Metal`（[为 Bare-Metal 编程：编译、链接与加载 (jyywiki.cn)](http://jyywiki.cn/OS/AbstractMachine/AM_Programs)），并如何在 `Bare-Metal`下编程，然后**实现 AbstractMachine 中 klib 中缺失的函数**   *[stdio.h(printf , sprintf...)、string.h(strlen,strcpy,strcmp,strcat...)、stdlib.h(malloc,free)]，可以先实现部分经常用到的（起码  printf  ），之后慢慢实现即可。

接下来便根据我们的裸机编程环境和我们实现的klib 以及 **提供好的五组基本api** 编写一个简易的小游戏即可。我选择的也是文档提供参考的弹球小游戏，w,a,s,d控制不同方向速度，g键可以暂停。

下面说一下我实验中的困难点和注意事项：

1. 在我们的abstarct-machine上如何编译与运行？
   因为我是**外校生**，也没有学习过Makefile，一开始并不知道具体要怎么做。。。后来提前看了下JYY老师b站上极限速通操作系统实验那节课（[极限速通操作系统实验 (内存管理、线程和信号量、进程和 kputc, sleep, fork) [南京大学2022操作系统-P22]_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1iY411A7w1/?spm_id_from=333.788&vd_source=33d3156975c92d1beb9e11e8b218f8b0)）  上肉眼观察了一下。。。

   根据写好的 `./abstact-machine/Makefile` 进行编译与运行，
   编译：在你要运行的那个项目下，本实验是amgame目录下使用 `make`命令即可
   ![[Pasted image 20220813231330.png]]
   运行：在你要运行的那个项目下，本实验是amgame目录下使用 `make run`命令
   ![[Pasted image 20220813231446.png]]
   如果想要可读性更好的阅读 `abstact-machine/`目录下 `Makefile`文件? 后来根据一节课上JYY讲授内容： 使用 `make html`命令即可，其实是非常巧妙的将makefile文件通过正则匹配转换成了可读性强的markdown文件，然后生成了直接阅读的html。
   更多相关编译选项可看： [操作系统的状态机模型 (操作系统的加载; thread-os 代码讲解) [南京大学2022操作系统-P9]_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV1yP4y1M7FE/?spm_id_from=333.788&vd_source=33d3156975c92d1beb9e11e8b218f8b0)
2. 实现 `printf`
   printf是我们之后进行这个实验必须的库函数，所以肯定要完成的。。。
   AbstractMachine提供了api：`putch`  :打印一个字符。
   但之后就会面临问题，我们先来看printf函数声明：
   `int printf(const char *fmt, ...)`
   `...`的可变参数中的内容要如何获取，我STFW，但发现要引入 `stdarg.h`中的 ` va_list` , `va_start()`等，一开始觉得不行，以为 `Bare-Metal`下编程不能引入其他的库文件，后来发现自己看文档没看到后面的内容。。。

   > 事实上，AbstractMachine 的程序运行在 [freestanding 的环境下](https://wiki.osdev.org/C_Library) (操作系统上的 C 程序运行在 hosted 环境下)：The `__STDC_HOSTED__` macro expands to `1` on hosted implementations, or 0 on freestanding ones. The freestanding headers are: `float.h`, `iso646.h`, `limits.h`, `stdalign.h`, `stdarg.h`, `stdbool.h`, `stddef.h`, `stdint.h`, and `stdnoreturn.h`. You should be familiar with these headers as they contain useful declarations you shouldn't do yourself. GCC also comes with additional freestanding headers for CPUID, SSE and such.
   > 这些头文件中包含了 freestanding 程序也可使用的声明。有兴趣的同学可以发现，可变参数经过预编译后生成了类似 `__builtin_va_arg` 的 builtin 调用，由编译器翻译成了特定的汇编代码。
   >

   解决了 `...`中的参数，之后呢？
   %d , %c , %s 分别如何实现。到这里已经能说明对于 `fmt`中的文字我们要逐个字符判断，遇到 `%` 之类的格式输出还要细分，因此整体采用switch形式的结构。
   一篇我觉得不错的博客可供参考：
   [(31条消息) 手把手教你实现printf函数（C语言方式）_老子姓李！的博客-CSDN博客_printf 实现](https://blog.csdn.net/qq_44078824/article/details/118440458?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522165815957216782350863814%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=165815957216782350863814&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-2-118440458-null-null.142^v32^new_blog_fixed_pos,185^v2^tag_show&utm_term=printf%E5%AE%9E%E7%8E%B0&spm=1018.2226.3001.4187)
3. 键盘io处理
   如何识别我们键盘每次按下，按了什么键，仔细观看源码和留下的框架代码，发现定义了每个按键的宏：

   ```
   #define AM_KEYS(_) \
   _(ESCAPE) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) 	_(F8) _(F9) _(F10)
    	... (自己去看吧)
   #define AM_KEY_NAMES(key) AM_KEY_##key,
   enum {
   AM_KEY_NONE = 0,
   AM_KEYS(AM_KEY_NAMES)
   };
   ```
   因为我在文档中并未找到设备文档，所以只能根据留下的框架代码照猫画虎，更改了一下，实现了自己的 `int readkey()`,然后将返回值送入 `kbd_event`,处理键盘事件，比如正确性标准规定的按下ESC退出游戏和按键对应的游戏的逻辑等。同时输出按键按下的结果，检验下自己实现的**printf** 的正确性，不断调试更改。查找阅读代码：`key_names[key]`是对应按键的字符串，打印它即可（框架代码已经写出来了）。
4. 图形界面处理
   阅读video.c 并不断运行建议的样例代码可推断出给我们留下的函数的作用，他们封装了gpu等硬件，让我们只用面对api与对象！
   `static void init()`初始化图形界面
   `void draw_tile(int x, int y, int w, int h, uint32_t color)` 在指定坐标上绘制一个宽度为 `WIDE（宏定义）`的颜色为 `color`的小方格。
   `splash`调用前两个函数绘制。
   困难处：
   a ) 正确理解一个按帧刷新的游戏的逻辑

   ```
   	while (1)
   	{
   		wait_for_frame(); //等待一帧的时间
   		while ((key = readkey()) != AM_KEY_NONE)
   		{
   			kbd_event(key); // 处理键盘事件
   		}
   		game_progress();		  // 处理一帧游戏逻辑，更新物体的位置等
   		screendraw();  // 重新绘制屏幕
   		next_frame += 1000 / FPS; // 计算下一帧的时间
   	}
   ```
   然后 `wait_for_frame();`如何实现？
   阅读了手册中推荐的一个jyy写好的ns模拟程序上剽窃了一下。。。
   b）弹球的边界处理
   因为这个，来回反复更改还是很痛苦的
   要注意的是，正常理解的w,a,s,d控制的方向和它对于gpu而言的坐标是不一样的！
   在这个实验中，y越大意味着显示上的那个点越低
   坐标是这样的:
   .---------->
   |
   |
   |
   V
   这个造成了一些困扰还有坐标和宽度以及坐标与宽度相乘等等对应关系需要去琢磨以及一些边界上-1等小细节还是很费时间和心态的。。。。
   还有帧率的挑选，一开始选的很高，结果画面很抽，弄了很久才知道是帧率的锅。。。。
5. 一些优秀的blogs(前面已经提到大部分)：
   a. [L0: 直接运行在硬件上的小游戏 (amgame) (jyywiki.cn)](http://jyywiki.cn/OS/2022/labs/L0) jyy的实验手册真的写的很好，膜拜，请**仔细**阅读，还有 `AbstractMachine`文档（[AbstractMachine: 抽象计算机 (jyywiki.cn)](http://jyywiki.cn/AbstractMachine/)。

   ##### RTFM！！！！！！

   b. 配置Abstract-Machine实验环境  [JYY操作系统实验L0 实验基础配置方案 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/499141891)   我是在linux虚拟机中做的，感觉比wsl好用些，但很多东西可以作为参考哈。
   c.  printf实现讲解   [(31条消息) 手把手教你实现printf函数（C语言方式）_老子姓李！的博客-CSDN博客_printf 实现](https://blog.csdn.net/qq_44078824/article/details/118440458?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522165815957216782350863814%2522%252C%2522scm%2522%253A%252220140713.130102334..%2522%257D&request_id=165815957216782350863814&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~all~sobaiduend~default-2-118440458-null-null.142^v32^new_blog_fixed_pos,185^v2^tag_show&utm_term=printf%E5%AE%9E%E7%8E%B0&spm=1018.2226.3001.4187)
