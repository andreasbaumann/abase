/* See LICENSE file for copyright and license details. */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-a] [<interface>] [up|down]\n", argv0);
}

#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP 1<<16
#endif
#ifndef IFF_DORMANT
#define IFF_DORMANT 1<<17
#endif
#ifndef IFF_ECHO
#define IFF_ECHO 1<<18
#endif

static void
print_interface(char *name)
{
	int fd;
	struct ifreq ifr;
	int first;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		weprintf("error in socket(AF_INTER, SOCK_DGRAM, 0) on interface '%s': %s\n", name, strerror(errno));
		return;
	}

	strncpy(ifr.ifr_name, name, IF_NAMESIZE);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		close(fd);
		weprintf("error in ioctl(SIOCGIFFLAGS) on interface '%s': %s\n", name, strerror(errno));
		return;
	}

	printf("%s: flags=%d<", name, ifr.ifr_flags);
	
	if (ifr.ifr_flags == 0) {
		printf(">");
	} else {
		first = 1;		
		if (ifr.ifr_flags & IFF_UP) {
			printf("%sUP", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_BROADCAST) {
			printf("%sBROADCAST", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_DEBUG) {
			printf("%sDEBUG", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_LOOPBACK) {
			printf("%sLOOPBACK", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_POINTOPOINT) {
			printf("%sPOINTOPOINT", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_RUNNING) {
			printf("%sRUNNING", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_NOARP) {
			printf("%sNOARP", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_PROMISC) {
			printf("%sPROMISC", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_NOTRAILERS) {
			printf("%sNOTRAILERS", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_ALLMULTI) {
			printf("%sALLMULTI", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_MASTER) {
			printf("%sMASTER", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_SLAVE) {
			printf("%sSLAVE", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_MULTICAST) {
			printf("%sMULTICAST", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_PORTSEL) {
			printf("%sPORTSEL", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_AUTOMEDIA) {
			printf("%sAUTOMEDIA", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_DYNAMIC) {
			printf("%sDYNAMIC", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_LOWER_UP) {
			printf("%sLOWER_UP", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_DORMANT) {
			printf("%sDORMANT", !first ? "," : ""); first = 0;
		}
		if (ifr.ifr_flags & IFF_ECHO) {
			printf("%sECHO", !first ? "," : ""); first = 0;
		}
		printf(">");
	}

	if (ioctl(fd, SIOCGIFMTU, &ifr) < 0) {
		close(fd);
		weprintf("error in ioctl(SIOCGIFMTU) on interface '%s': %s\n", name, strerror(errno));
		return;
	}
 	printf("  mtu %d\n", ifr.ifr_mtu);

	char ip_addr[INET_ADDRSTRLEN];
	if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
		struct sockaddr_in* addr = (struct sockaddr_in*)&ifr.ifr_addr;
		inet_ntop(AF_INET, &addr->sin_addr, ip_addr, INET_ADDRSTRLEN);
	} else {
		ip_addr[0] = '\0';
	}
	
	char ip_netmask[INET_ADDRSTRLEN];
	if (ioctl(fd, SIOCGIFNETMASK, &ifr) == 0) {
		struct sockaddr_in* netmask = (struct sockaddr_in*)&ifr.ifr_netmask;
		inet_ntop(AF_INET, &netmask->sin_addr, ip_netmask, INET_ADDRSTRLEN);
	} else {
		ip_netmask[0] = '\0';
	}

	char ip_broadcast[INET_ADDRSTRLEN];
	if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == 0) {
		struct sockaddr_in* broadcast = (struct sockaddr_in*)&ifr.ifr_broadaddr;
		inet_ntop(AF_INET, &broadcast->sin_addr, ip_broadcast, INET_ADDRSTRLEN);
	} else {
		ip_broadcast[0] = '\0';
	}
	
	char ip_hwaddr[32];
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
		memcpy(ip_hwaddr, ifr.ifr_hwaddr.sa_data, 8);
	} else {
		memset(ip_hwaddr, 0, 32);
	}

	printf("        inet %s  netmask %s  broadcast %s\n",
	       ip_addr, ip_netmask, ip_broadcast);
	printf("        ether ");
	first = 1;
	for (int i = 0; i <6; i++) {
		printf("%s%02x", ( !first ) ? ":" : "", ip_hwaddr[i] & 0xFF); first = 0;
	}
	puts("");

	close(fd);
}

static void
set_up_down(char *name, int up)
{
	int fd;
	struct ifreq ifr;
	
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		weprintf("error in socket(AF_INTER, SOCK_DGRAM, 0) on interface '%s': %s\n", name, strerror(errno));
		return;
	}

	strncpy(ifr.ifr_name, name, IF_NAMESIZE);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		close(fd);
		weprintf("error in ioctl(SIOCGIFFLAGS) on interface '%s': %s\n", name, strerror(errno));
		return;
	}
	
	if (up) {
		ifr.ifr_flags |= IFF_UP;
	} else  {
		ifr.ifr_flags &= ~IFF_UP;
	}

	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		close(fd);
		weprintf("error in ioctl(SIOCGIFFLAGS) on interface '%s': %s\n", name, strerror(errno));
		return;
	}
}

int
main(int argc, char *argv[])
{
	int show_all = 0;
	
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
	} else {
		if (argc == 1) {
			print_interface(argv[0]);
		} else if (argc == 2) {
			if (strcmp(argv[1], "up") == 0) {
				set_up_down(argv[0], 1);
			} else if (strcmp(argv[1], "down") == 0) {
				set_up_down(argv[0], 0);
			} else {
				weprintf("Expecting 'up' or 'down'\n");
				usage();
			}
		} else {
			usage();
		}
	}
	
	return 0;
}
