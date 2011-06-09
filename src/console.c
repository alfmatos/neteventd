#include <netevent/console.h>

static int color_output=0;

void console_exit_cleanup(void)
{
	fflush(stdout);
	printf("\e[0m");
}

void colorize(char * cmd, int color)
{
	sprintf(cmd, "\e[%dm", color + 30);
}

/**
 * @short Whether or not the terminal is "dumb"(no color support)
 * @return 1 if it is dumb, 0 otherwise
 */
static int is_terminal_dumb()
{
	char *term = getenv("TERM");

	if ( term && strncmp(term, "TERM", 4) == 0 ) {
		return 1;
	}

	return 0;
}

/**
 * @short Enable colored output
 *
 * i.e. it just sets color_output to 1, but does some additional
 * sanity checks
 *
 * @return 0 if color was not enabled
 */
int enable_color_output(void)
{

	if (isatty( fileno(stdout)) == 0 ) {
		color_output = 0;
	} else if ( is_terminal_dumb() ) {
		color_output = 0;
	}  else {
		color_output = 1;
	}

	if ( color_output == 0 ) {
		fprintf(stderr, "Colored output doesn't seem to be supported by the terminal...disabling\n");
	}

	return color_output;
}

int eprintf(int color, char *format, ...)
{
	va_list argp;
	char msg[2048], timestamp[20];
	struct timeval tv;
	struct tm *t;
	char fg[13]="", fg_reset[]="\e[0m";

	gettimeofday(&tv, NULL);
	t = localtime(&tv.tv_sec);

	sprintf(timestamp,"[%02d:%02d:%02d.%06ld]",
		t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);

	va_start(argp, format);
	vsnprintf(msg, sizeof(msg), format, argp);
	va_end(argp);

	if(color_output && (color != NONE)) {
		colorize(fg, color);
	}

	printf("%s%s %s%s", fg, timestamp, msg, fg_reset);
	fflush(stdout);

	return 0;
}
