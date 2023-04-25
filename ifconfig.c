/* See LICENSE file for copyright and license details. */
#include <net/if.h>
#include <stdio.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-a]\n", argv0);
}

static void
print_interface(char *name)
{
	printf("%s\n", name);
}

int
main(int argc, char *argv[])
{
	int show_all = 0;
	//~ struct ifreq ifr;
	
	ARGBEGIN {
	case 'a':
		show_all = 1;
		break;
	default:
		usage();
	} ARGEND
	
	if (show_all) {
		struct if_nameindex *interfaces, *interface;
		
		interfaces = if_nameindex();
		if (interfaces != NULL) {
			for (interface = interfaces; (interface->if_index != 0 || interface->if_name != NULL); interface++) {
				print_interface(interface->if_name);
			}
			if_freenameindex(interfaces);
		}
	}
	
	return 0;
}
