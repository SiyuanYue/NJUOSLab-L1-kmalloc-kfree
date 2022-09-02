#include <game.h>

#define KEYNAME(key) \
	[AM_KEY_##key] = #key,
static const char *key_names[] = {
	AM_KEYS(KEYNAME)};

void print_key()
{
	AM_INPUT_KEYBRD_T event = {.keycode = AM_KEY_NONE};
	ioe_read(AM_INPUT_KEYBRD, &event);
	if (event.keycode != AM_KEY_NONE && event.keydown)
	{
		if(event.keycode==AM_KEY_ESCAPE)
			halt(0);
		printf("Key pressed: ");
		printf("%s\n",key_names[event.keycode]);
	}
}

int readkey()
{
	AM_INPUT_KEYBRD_T event = {.keycode = AM_KEY_NONE};
	ioe_read(AM_INPUT_KEYBRD, &event);
	if(event.keycode != AM_KEY_NONE && event.keydown)
	{
		return event.keycode;
	}
	else
	{
		return AM_KEY_NONE;
	}
}