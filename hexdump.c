/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-C] [file ...]\n", argv0);
}

#define BYTES_PER_LINE 16

int
main(int argc, char *argv[])
{
	int fd;
	char c;
	int nread;
	int pos;
	int lastpos;
	int i;
	char buf[BYTES_PER_LINE];

	ARGBEGIN {
	case 'v':
		break;
	case 'C':
		break;
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

	lastpos = 0;
	pos = 0;
	while ((nread=read(fd, &c, 1)) >= 0) {
		if (nread > 0) {
			if (pos % BYTES_PER_LINE == 0) {
				printf("%08x ", pos);
			}
			printf(" %02x", c & 0xFF );
			buf[pos%BYTES_PER_LINE] = c;
			pos++;
		} else {
			if (lastpos == 0) {
				lastpos = pos;
			}
			printf("   ");
			pos++;
		}
		if (pos % BYTES_PER_LINE == BYTES_PER_LINE/2) {
			printf(" ");
		}
		if (pos % BYTES_PER_LINE == 0) {
			printf("  |");
			if (nread == 0) {
				pos = lastpos;
			}
			for (i = 0; i <= (pos+BYTES_PER_LINE-1) % BYTES_PER_LINE; i++) {
				if (isprint(buf[i]) ) {
					putchar(buf[i]);
				} else {
					putchar('.');
				}
			}
			puts("|");
			if (nread == 0) {
				goto END;
			}
		}
	}
END:
	printf("%08x\n", pos);


	if (fd > STDERR_FILENO) {
		close(fd);
	}
	
	return 0;
}
