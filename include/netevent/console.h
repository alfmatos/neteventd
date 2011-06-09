#ifndef __NETEVENT_CONSOLE__
#define __NETEVENT_CONSOLE__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#define BLACK 		0
#define RED 		1
#define GREEN 		2
#define YELLOW 		3
#define BLUE 		4
#define MAGENTA 	5
#define CYAN 		6
#define WHITE 		7
#define NONE		128

#define OPT_UNKNOWN 	0
#define OPT_COLOR 	1

int enable_color_output(void);
void console_exit_cleanup(void);

int eprintf(int color, char *format, ...);

#define tprintf(args...) eprintf(NONE, ##args)

#endif /* __NETVENT_CONSOLE__ */
