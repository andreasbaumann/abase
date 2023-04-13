/* See LICENSE file for copyright and license details. */

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int ret = 0;

	ARGBEGIN {
	default:
		usage();
	} ARGEND
		
	return ret;
}
