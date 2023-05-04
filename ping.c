/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "util.h"

static volatile int terminate = 0;

static void
usage(void)
{
	eprintf("usage: %s <destination>\n", argv0);
}

static void
terminateHandler(int sig)
{
	terminate = 1;
}

/* as per RFC 1035 section 2.3.4 it is reasonable to assume host names
 * are not longer than 255 characters */
#define FQDN_LEN 255

#define PING_PACKET_SIZE 64
struct ping_packet {
	struct icmphdr header;
	char msg[PING_PACKET_SIZE-sizeof(struct icmphdr)];
};

/* see Stevens Unix Networking Programming Vol. 1, p. 672 */
static
unsigned short checksum(void *b, int buflen)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result = 0;

	for (sum = 0; buflen > 1; buflen -= 2) {
		sum += *buf++;
	}
	if (buflen == 1) {
		sum += *(unsigned char*)buf;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >>16);
	result = ~sum;
	
	return result;
}

int
main(int argc, char *argv[])
{
	struct addrinfo hints;
	char *host;
	struct addrinfo *addresses, *address;
	struct sockaddr *ping_address;
	struct sockaddr_in sending_address;
	int res, fd;
	struct timespec time_start, time_end;

	ARGBEGIN {
	default:
		usage();
	} ARGEND
	
	if (argc != 1) {
		weprintf("Expecting exactly one 'destination' to ping\n");
		usage();
	}
	host = argv[0];
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET; /* IPv4 for now only */
	hints.ai_socktype = SOCK_RAW;
	hints.ai_flags = AI_CANONNAME;
	hints.ai_protocol = IPPROTO_ICMP;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	if ((res=getaddrinfo(host, NULL, &hints, &addresses)) != 0) {
		weprintf("error calling getaddrinfo for '%s': %s\n", host, gai_strerror(res));
		return 1;
	}
	
	char ip_addr[INET_ADDRSTRLEN];
	char canon_host[FQDN_LEN];
	ping_address = NULL;
	for (address = addresses; address != NULL; address = address->ai_next) {
		struct sockaddr_in* addr = (struct sockaddr_in*)address->ai_addr;
		inet_ntop(AF_INET, &addr->sin_addr, ip_addr, INET_ADDRSTRLEN);
		strncpy(canon_host, address->ai_canonname, sizeof(canon_host));

		fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (fd == -1) {
			if (errno == EPERM) {
				weprintf("Permission denied, are you root?\n");
				continue;
			}
			weprintf("socket failed: %s\n", strerror(errno));
			continue;
		} else {
			ping_address = address->ai_addr;
		}
		
		break;
	}
	
	if (ping_address == NULL) {
		return 1;
	}
	
	signal(SIGINT, terminateHandler);
	
	printf("PING %s (%s)\n", canon_host, ip_addr);

	int ttl_val = 255;
	if (setsockopt(fd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0) {
		freeaddrinfo(addresses);
		close(fd);
		eprintf("Unable to set TTL on socket: %s\n", host, strerror(errno));
	}

	struct timeval tv_out;
	tv_out.tv_sec = 1;
	tv_out.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof(tv_out)) < 0) {
		freeaddrinfo(addresses);
		close(fd);
		eprintf("Unable to set receive time on socket: %s\n", host, strerror(errno));
	}
	
	int counter = 0;
	int sent = 0;
	int received = 0;
	while (!terminate) {
		struct ping_packet packet;
		
		memset(&packet, 0, sizeof(packet));
		packet.header.type = ICMP_ECHO;
		packet.header.code = 0; /* ECHO */
		packet.header.un.echo.id = getpid();
		packet.header.un.echo.sequence = counter;
		for (int i = 0; i < sizeof(packet.msg)-1; i++) {
			packet.msg[i] = counter+i;
		}
		packet.header.checksum = 0; /* set to 0 for checksum computation */
		packet.header.checksum = checksum(&packet, sizeof(packet));
		counter++;

		sleep(1);
		if (terminate) {
			continue;
		}

		clock_gettime(CLOCK_MONOTONIC, &time_start);
		
		if (sendto(fd, &packet, sizeof(packet), 0, ping_address, sizeof(*ping_address)) < 0) {
			weprintf("Unable to send packet to '%s': %s\n", host, strerror(errno));
			sent = 0;
		} else {
			sent = 1;
		}

		unsigned int address_len = sizeof(sending_address);
		if (recvfrom(fd, &packet, sizeof(packet), 0, (struct sockaddr *)&sending_address, &address_len) <= 0) {
			if (errno == EAGAIN || errno == EINTR) {
				// ok, not reachable or interrupted by Ctrl-C
			} else {
				freeaddrinfo(addresses);
				close(fd);
				eprintf("Unable to receive packet from '%s': %s\n", host, strerror(errno));
			}
		} else {
			if (sent) {
				if (packet.header.type != 69 || packet.header.code != 0) {
					freeaddrinfo(addresses);
					close(fd);
					eprintf("Received unknown ICMP package with code=%d and type=%d from '%s'",
						packet.header.code, packet.header.type, host);
				} else {
					inet_ntop(AF_INET, &sending_address.sin_addr, ip_addr, INET_ADDRSTRLEN);

					clock_gettime(CLOCK_MONOTONIC, &time_end);
					double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
					long double rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;

					printf("%ld bytes from %s (%s): icmp_seq=%d ttl=%d time=%0.3Lf ms\n",
						sizeof(packet), host, ip_addr, counter, ttl_val, rtt_msec);
					received++;
				}
			}
		}
	}
	
	printf("--- %s ping statistics ---\n"
		"%d packets transmitted, %d received , %0.0f%% packet loss\n",
		canon_host, counter, received,
		((counter - received)/counter) * 100.0); 

	freeaddrinfo(addresses);
	
	close(fd);

	return 0;
}
