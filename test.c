 #define _GNU_SOURCE
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define ROWS 64
#define COLS 32

unsigned dhost[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

int main(int argc, char **argv) {
	struct ifreq if_idx;
	struct sockaddr_ll sock_addr;
	struct mmsghdr msgs[ROWS + 2];
	struct iovec iovecs[(2 * ROWS) + 2];
	struct ether_header *header;
	unsigned char buffer[ROWS][COLS][3];
	char iface[] = "ens33";
	unsigned char *ptr;
	int fd, x, y;
	
	ptr = (unsigned char *) malloc(112);
	iovecs[0].iov_base = ptr;
	iovecs[0].iov_len = 112;
	memset(ptr, 0, 112);
	header = (struct ether_header *) ptr;
	header->ether_type = htons(0x0107);
	header->ether_shost[0] = 0x22;
	header->ether_shost[1] = 0x22;
	header->ether_shost[2] = 0x33;
	header->ether_shost[3] = 0x44;
	header->ether_shost[4] = 0x55;
	header->ether_shost[5] = 0x66;
	header->ether_dhost[0] = 0x11;
	header->ether_dhost[1] = 0x22;
	header->ether_dhost[2] = 0x33;
	header->ether_dhost[3] = 0x44;
	header->ether_dhost[4] = 0x55;
	header->ether_dhost[5] = 0x66;
	ptr[sizeof(struct ether_header) + 21] = 0xFF;
	ptr[sizeof(struct ether_header) + 22] = 0x05;
	ptr[sizeof(struct ether_header) + 24] = 0xFF;
	ptr[sizeof(struct ether_header) + 25] = 0xFF;
	ptr[sizeof(struct ether_header) + 26] = 0xFF;
	msgs[ROWS].msg_hdr.msg_iov = &iovecs[0];
	msgs[ROWS].msg_hdr.msg_iovlen = 1;
	
	ptr = (unsigned char *) malloc(77);
	iovecs[1].iov_base = ptr;
	iovecs[1].iov_len = 77;
	memset(ptr, 0, 77);
	header = (struct ether_header *) ptr;
	header->ether_type = htons(0x0AFF);
	header->ether_shost[0] = 0x22;
	header->ether_shost[1] = 0x22;
	header->ether_shost[2] = 0x33;
	header->ether_shost[3] = 0x44;
	header->ether_shost[4] = 0x55;
	header->ether_shost[5] = 0x66;
	header->ether_dhost[0] = 0x11;
	header->ether_dhost[1] = 0x22;
	header->ether_dhost[2] = 0x33;
	header->ether_dhost[3] = 0x44;
	header->ether_dhost[4] = 0x55;
	header->ether_dhost[5] = 0x66;
	ptr[sizeof(struct ether_header) + 0] = 0xFF;
	ptr[sizeof(struct ether_header) + 1] = 0xFF;
	ptr[sizeof(struct ether_header) + 2] = 0xFF;
	msgs[ROWS + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[ROWS + 1].msg_hdr.msg_iovlen = 1;
	
	for (x = 0; x < ROWS; x++) {
		ptr = (unsigned char *) malloc(sizeof(struct ether_header) + 7);
		iovecs[x * 2 + 2].iov_base = ptr;
		iovecs[x * 2 + 2].iov_len = sizeof(struct ether_header) + 7;
		memset(ptr, 0, sizeof(struct ether_header) + 7);
		header = (struct ether_header *) ptr;
		header->ether_type = htons(0x5500);
		header->ether_shost[0] = 0x22;
		header->ether_shost[1] = 0x22;
		header->ether_shost[2] = 0x33;
		header->ether_shost[3] = 0x44;
		header->ether_shost[4] = 0x55;
		header->ether_shost[5] = 0x66;
		header->ether_dhost[0] = 0x11;
		header->ether_dhost[1] = 0x22;
		header->ether_dhost[2] = 0x33;
		header->ether_dhost[3] = 0x44;
		header->ether_dhost[4] = 0x55;
		header->ether_dhost[5] = 0x66;
		ptr[sizeof(struct ether_header) + 1] = x >> 8;
		ptr[sizeof(struct ether_header) + 0] = x & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = COLS >> 8;
		ptr[sizeof(struct ether_header) + 4] = COLS & 0xFF;
		ptr[sizeof(struct ether_header) + 5] = 0x08;
		ptr[sizeof(struct ether_header) + 6] = 0x88;
		iovecs[x * 2 + 3].iov_base = &buffer[x][0][0];
		iovecs[x * 2 + 3].iov_len = COLS * 3;
		msgs[x].msg_hdr.msg_iov = &iovecs[x * 2 + 2];
		msgs[x].msg_hdr.msg_iovlen = 2;
	}
	
	if ((fd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		printf("error no= %d, ERROR = %s \n",errno,strerror(errno));
		return -1;
	}
	
	memset(&if_idx, 0, sizeof(struct ifreq));
	strcpy(if_idx.ifr_name, iface);
	if (ioctl(fd, SIOCGIFINDEX, &if_idx) < 0) {
		printf("error no= %d, ERROR = %s \n",errno,strerror(errno));
		return -2;
	}
	
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_ifindex = if_idx.ifr_ifindex;
	sock_addr.sll_halen = ETH_ALEN;
	memcpy(sock_addr.sll_addr, dhost, 6);

	if (bind(fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
		printf("error no= %d, ERROR = %s \n",errno,strerror(errno));
        	return -3;
    	}
    	
    	for (x = 0; x < COLS; x++) {
    		for (y = 0; y < ROWS; y++) {
    			buffer[y][x][0] = 0xFF;
    			buffer[y][x][1] = 0xFF;
    			buffer[y][x][2] = 0xFF;
    		}
    	}
    	
    	if (sendmmsg(fd, msgs, ROWS + 2, 0) != ROWS + 2) {
		printf("error no= %d, ERROR = %s \n",errno,strerror(errno));
    		return -4;
    	}

	return 0;
}
