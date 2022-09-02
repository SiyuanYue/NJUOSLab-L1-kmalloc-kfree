#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

void splash();
void central(uint32_t color);
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}
int readkey();
void draw_tile(int x, int y, int w, int h, uint32_t color);
void kbd_event(int key);
void game_progress();
double uptime();