#include <game.h>
// Operating system is a C program!
#define FPS 45
#define KEYNAME(key) \
	[AM_KEY_##key] = #key,
static const char *key_names[] = {
	AM_KEYS(KEYNAME)};
#define color_put 0xFFFFFF
#define SIDE 16
static double x,y;
static double vx=0,vy=0;
static int next_frame;
extern int w,h;
static inline int uptime_ms() {
  return io_read(AM_TIMER_UPTIME).us / 1000;
}
void screendraw()
{
	splash();
	draw_tile(x*SIDE,y*SIDE,SIDE,SIDE,color_put);
}
void kbd_event(int key)
{
	if(key==AM_KEY_ESCAPE)
		halt(0);
	else if(key==AM_KEY_W)
	{
		vy-=3;
		//y--;
	}
	else if(key==AM_KEY_S)
	{
		vy+=3;
		//y++;
	}
	else if(key==AM_KEY_D)
	{
		vx+=3;
		//x++;
	}
	else if(key==AM_KEY_A)
	{
		vx-=3;
		//x--;
	}
	else if(key==AM_KEY_G)
	{
		vx=0;
		vy=0;
	}
	printf("Key pressed: %s\n", key_names[key]);
}
void game_progress()
{
	x=x+vx/(double)FPS;
	y=y+vy/(double)FPS;
	if(x*SIDE<=0||x>=w/SIDE-1)
	{
		vx=-vx;
		if(x<=0)
			{
				x=0;
				assert(vx>=0);
			}
		else
		{
			x=w/SIDE-1;
		}	
	}	
	if(y*SIDE<=0||y>=h/SIDE-1)
	{
		vy=-vy;
		if(y*SIDE<=0)
			{
				y=0;
				assert(vy>=0);
			}
		else
			y=h/SIDE-1;
	}
	// if(x*SIDE<=0)
	// 	x=0;
	// if(x*SIDE>=w)
	// 	x=w/SIDE-1;
	// if(y*SIDE<=0)
	// 	y=0;
	// if(y*SIDE>=h)
	// 	y=h/SIDE-1;
}
void wait_for_frame() {
  int cur = uptime_ms();
  while (cur < next_frame) {
    cur = uptime_ms();
  }
  next_frame = cur;
}
int main(const char *args)
{
	ioe_init();
	puts("mainargs = \"");
	puts(args); // make run mainargs=xxx
	puts("\"\n");
	splash();
	printf("Press any key to see its key code...\n");
	int key;
	x=w/SIDE/2;// init x
	y=h/SIDE/2;// init y
	draw_tile((int)x*SIDE,(int)y*SIDE,SIDE,SIDE,color_put);
	vx=0;vy=0;// init vx,vy
	next_frame=0;
	while (1)
	{
		wait_for_frame();
		while ((key = readkey()) != AM_KEY_NONE)
		{
			kbd_event(key); // 处理键盘事件
		}
		game_progress();		  // 处理一帧游戏逻辑，更新物体的位置等
		screendraw();  // 重新绘制屏幕
		next_frame += 1000 / FPS; // 计算下一帧的时间
		
	}
	return 0;
}
