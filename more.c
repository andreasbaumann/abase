/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include "util.h"

#define COLUMNS 80
#define ROWS 25
#define BUFSIZE 10

static void
usage(void)
{
	eprintf("usage: %s [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int fd;
	char c;
	int col;
	int row;
	int buf[BUFSIZE];

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc < 1 && isatty(STDIN_FILENO)) {
		usage();
	}
	
	if (isatty(STDIN_FILENO)) {
		if ((fd = open(*argv, O_RDONLY)) < 0) {
			weprintf("open %s:", *argv);
			return 0;
			
		}
	} else {
		fd = STDIN_FILENO;
	}
	
	col = 0;
	row = 0;
	while (read(fd, &c, 1) != 0) {
		switch (c) {
			case '\r':
				col = 0;
				break;
			
			case '\n':
				col = 0;
				row++;
				break;
			
			case '\t':
				col = ((col+1) | 0x07)+1;
				break;
			
			case '\b':
				if (col>0) {
					col--;
				}
				break;
				
			default:
				col++;
				break;
		}
		putchar(c);
		
		if (row<ROWS) {
			continue;
		}
		
		puts("--more--");
		
		if (read(STDIN_FILENO, &buf, BUFSIZE) < 0) {
			goto END;
		}
		
		col = 0;
		row = 0;		
	}
	
END:
	if (fd > STDERR_FILENO) {
		close(fd);
	}
	
	return 0;
}
