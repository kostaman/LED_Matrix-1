/* 
 * File:   Matrix.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */
 
#include <linux/if_packet.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

Matrix::Matrix(const char *iface) {
	fd = -1;
	buffer = new Matrix_RGB_t[rows * cols];
	struct ifreq if_idx;
	struct sockaddr_ll sock_addr;
	unsigned dhost[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
	
	if ((fd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1)
		throw errno;
	
	memset(&if_idx, 0, sizeof(struct ifreq));
	strcpy(if_idx.ifr_name, iface);
	if (ioctl(fd, SIOCGIFINDEX, &if_idx) < 0)
		throw errno;
	
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_ifindex = if_idx.ifr_ifindex;
	sock_addr.sll_halen = ETH_ALEN;
	memcpy(sock_addr.sll_addr, dhost, 6);

	if (bind(fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) 
		throw errno;
}

Matrix::~Matrix() {
	delete[] buffer;
	if (fd >= 0)
		close(fd);
}

inline void Matrix::map(uint32_t *x, uint32_t *y) {
	uint32_t x2 = *x % cols;
	uint32_t y2 = (*x / cols * 16) + (*y % 16);
	*x = x2;
	*y = y2;
}

inline void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	map(&x, &y);
	y %= rows;
	x %= cols;
    	*(buffer + (y * cols) + x) = pixel;
}

inline void Matrix::fill(Matrix_RGB_t pixel) {
    	for (uint32_t x = 0; x < cols; x++)
    		for (uint32_t y = 0; y < rows; y++)
    			*(buffer + (y * cols) + x) = pixel;
}

inline void Matrix::clear() {
	fill(Matrix_RGB_t(0, 0, 0));	
}

inline void Matrix::send_frame() {
	struct mmsghdr msgs[rows + 2];
	struct iovec iovecs[(2 * rows) + 2];
	struct ether_header *header;
	unsigned char *ptr;
	int x;
	
	memset(msgs, 0, sizeof(msgs));
	memset(iovecs, 0, sizeof(iovecs));
	
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
	msgs[rows].msg_hdr.msg_iov = &iovecs[0];
	msgs[rows].msg_hdr.msg_iovlen = 1;
	
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
	msgs[rows + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[rows + 1].msg_hdr.msg_iovlen = 1;
	
	for (x = 0; x < rows; x++) {
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
		ptr[sizeof(struct ether_header) + 3] = cols >> 8;
		ptr[sizeof(struct ether_header) + 4] = cols & 0xFF;
		ptr[sizeof(struct ether_header) + 5] = 0x08;
		ptr[sizeof(struct ether_header) + 6] = 0x88;
		iovecs[x * 2 + 3].iov_base = (buffer + (x * cols));
		iovecs[x * 2 + 3].iov_len = cols * sizeof(Matrix_RGB_t);
		msgs[x].msg_hdr.msg_iov = &iovecs[x * 2 + 2];
		msgs[x].msg_hdr.msg_iovlen = 2;
	}
	
	if (sendmmsg(fd, msgs, rows + 2, 0) != rows + 2)
		throw errno;
}

inline void Matrix::send_frame(uint16_t id) {
	struct mmsghdr msgs[rows + 2];
	struct iovec iovecs[(2 * rows) + 2];
	struct ether_header *header;
	unsigned char *ptr;
	int x;
	
	id &= 0xFFF;
	
	memset(msgs, 0, sizeof(msgs));
	memset(iovecs, 0, sizeof(iovecs));
	
	ptr = (unsigned char *) malloc(116);
	iovecs[0].iov_base = ptr;
	iovecs[0].iov_len = 116;
	memset(ptr, 0, 116);
	header = (struct ether_header *) ptr;
	header->ether_type = htons(0x8100);
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
	ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
	ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
	ptr[sizeof(struct ether_header) + 2] = htons(0x0107) & 0xFF;
	ptr[sizeof(struct ether_header) + 3] = htons(0x0107) >> 8;
	ptr[sizeof(struct ether_header) + 25] = 0xFF;
	ptr[sizeof(struct ether_header) + 26] = 0x05;
	ptr[sizeof(struct ether_header) + 28] = 0xFF;
	ptr[sizeof(struct ether_header) + 29] = 0xFF;
	ptr[sizeof(struct ether_header) + 30] = 0xFF;
	msgs[rows].msg_hdr.msg_iov = &iovecs[0];
	msgs[rows].msg_hdr.msg_iovlen = 1;
	
	ptr = (unsigned char *) malloc(81);
	iovecs[1].iov_base = ptr;
	iovecs[1].iov_len = 81;
	memset(ptr, 0, 81);
	header = (struct ether_header *) ptr;
	header->ether_type = htons(0x8100);
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
	ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
	ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
	ptr[sizeof(struct ether_header) + 2] = htons(0x0AFF) & 0xFF;
	ptr[sizeof(struct ether_header) + 3] = htons(0x0AFF) >> 8;
	ptr[sizeof(struct ether_header) + 4] = 0xFF;
	ptr[sizeof(struct ether_header) + 5] = 0xFF;
	ptr[sizeof(struct ether_header) + 6] = 0xFF;
	msgs[rows + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[rows + 1].msg_hdr.msg_iovlen = 1;
	
	for (x = 0; x < rows; x++) {
		ptr = (unsigned char *) malloc(sizeof(struct ether_header) + 11);
		iovecs[x * 2 + 2].iov_base = ptr;
		iovecs[x * 2 + 2].iov_len = sizeof(struct ether_header) + 11;
		memset(ptr, 0, sizeof(struct ether_header) + 11);
		header = (struct ether_header *) ptr;
		header->ether_type = htons(0x8100);
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
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
		ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
		ptr[sizeof(struct ether_header) + 2] = htons(0x5500) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x5500) >> 8;
		ptr[sizeof(struct ether_header) + 5] = x >> 8;
		ptr[sizeof(struct ether_header) + 4] = x & 0xFF;
		ptr[sizeof(struct ether_header) + 7] = cols >> 8;
		ptr[sizeof(struct ether_header) + 8] = cols & 0xFF;
		ptr[sizeof(struct ether_header) + 9] = 0x08;
		ptr[sizeof(struct ether_header) + 10] = 0x88;
		iovecs[x * 2 + 3].iov_base = (buffer + (x * cols));
		iovecs[x * 2 + 3].iov_len = cols * sizeof(Matrix_RGB_t);
		msgs[x].msg_hdr.msg_iov = &iovecs[x * 2 + 2];
		msgs[x].msg_hdr.msg_iovlen = 2;
	}
	
	if (sendmmsg(fd, msgs, rows + 2, 0) != rows + 2)
		throw errno;
}
