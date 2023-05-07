/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s <module>\n", argv0);
}

int
main(int argc, char *argv[])
{
	char *module;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc != 1) {
		weprintf("Expecting exactly one 'module' to remove\n");
		usage();
	}
	module = argv[0];
	
	if (syscall(__NR_delete_module, module, O_NONBLOCK) < 0)
		eprintf("delete_module:");

	return 0;
}
