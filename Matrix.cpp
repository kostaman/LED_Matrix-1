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
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <cmath>
#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

Matrix::Matrix(const char *iface, uint32_t r, uint32_t c) : rows(r), cols(c) {
	int f;
	uint32_t *ptr;
	struct ifreq if_idx;
	struct sockaddr_ll sock_addr;
	unsigned dhost[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
	
	brightness = 0xFF;
	b_raw = 0xFF;
	
	if ((f = open("/tmp/LED_Matrix.mem", O_CREAT | O_RDWR, 0666)) < 0)
		if ((f = open("/tmp/LED_Matrix.mem", O_RDWR, 0666)) < 0)
			throw errno;
	
	if (chmod("/tmp/LED_Matrix.mem", 0666) < 0)
		throw errno;
		
	if (ftruncate(f, 4 + (cols * rows * sizeof(Matrix_RGB_t))) < 0)
		throw errno;
	
	ptr = (uint32_t *) mmap(NULL, 4 + (cols * rows * sizeof(Matrix_RGB_t)), PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
	
	buffer = (Matrix_RGB_t *) (ptr + 1);
	
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

inline void Matrix::map(uint32_t *x, uint32_t *y) {
	uint32_t x2 = *x % cols;
	uint32_t y2 = (*x / cols * 16) + (*y % 16);
	*x = x2;
	*y = y2;
}

void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	map(&x, &y);
	y %= rows;
	x %= cols;
    	*(buffer + (y * cols) + x) = pixel;
}

void Matrix::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	y %= rows;
	x %= cols;
    	*(buffer + (y * cols) + x) = pixel;
}

void Matrix::fill(Matrix_RGB_t pixel) {
    	for (uint32_t x = 0; x < cols; x++)
    		for (uint32_t y = 0; y < rows; y++)
    			*(buffer + (y * cols) + x) = pixel;
}

void Matrix::clear() {
	fill(Matrix_RGB_t(0, 0, 0));	
}

void Matrix::set_brightness(uint8_t b) {
	b %= 101;
	b_raw = round(b / 100.0 * 255.0);
	brightness = round(pow(b / 100.0, 0.405) * 255.0);
}

static void set_address(struct ether_header *header) {
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
}

void Matrix::send_frame() {
	send_frame(false, 0);
}

void Matrix::send_frame(uint16_t vlan_id) {
	send_frame(true, vlan_id);
}

void Matrix::send_frame(bool vlan, uint16_t id) {
	uint32_t offset = 0;
	struct mmsghdr msgs[rows + 2];
	struct iovec iovecs[(2 * rows) + 2];
	struct ether_header *header;
	unsigned char *ptr;
	int x;
	
	id &= 0xFFF;
	if (vlan)
		offset = 4;
	
	memset(msgs, 0, sizeof(msgs));
	memset(iovecs, 0, sizeof(iovecs));
	
	ptr = (unsigned char *) malloc(112 + offset);
	iovecs[0].iov_base = ptr;
	iovecs[0].iov_len = 112 + offset;
	memset(ptr, 0, 112 + offset);
	header = (struct ether_header *) ptr;
	if (!vlan)
		header->ether_type = htons(0x0107);
	else
		header->ether_type = htons(0x8100);
	set_address(header);
	if (vlan) {
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
		ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
		ptr[sizeof(struct ether_header) + 2] = htons(0x0107) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0107) >> 8;
	}
	ptr[sizeof(struct ether_header) + 21 + offset] = b_raw;
	ptr[sizeof(struct ether_header) + 22 + offset] = 0x05;
	ptr[sizeof(struct ether_header) + 24 + offset] = b_raw;
	ptr[sizeof(struct ether_header) + 25 + offset] = b_raw;
	ptr[sizeof(struct ether_header) + 26 + offset] = b_raw;
	msgs[rows].msg_hdr.msg_iov = &iovecs[0];
	msgs[rows].msg_hdr.msg_iovlen = 1;
	
	ptr = (unsigned char *) malloc(77 + offset);
	iovecs[1].iov_base = ptr;
	iovecs[1].iov_len = 77 + offset;
	memset(ptr, 0, 77 + offset);
	header = (struct ether_header *) ptr;
	if (!vlan)
		header->ether_type = htons(0x0A00 + brightness);
	else
		header->ether_type = htons(0x8100);
	set_address(header);
	if (vlan) {
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
		ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
		ptr[sizeof(struct ether_header) + 2] = htons(0x0A00 + brightness) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0A00 + brightness) >> 8;
	}
	ptr[sizeof(struct ether_header) + offset] = brightness;
	ptr[sizeof(struct ether_header) + 1 + offset] = brightness;
	ptr[sizeof(struct ether_header) + 2 + offset] = 0xFF;
	msgs[rows + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[rows + 1].msg_hdr.msg_iovlen = 1;
	
	for (x = 0; x < rows; x++) {
		ptr = (unsigned char *) malloc(sizeof(struct ether_header) + 7 + offset);
		iovecs[x * 2 + 2].iov_base = ptr;
		iovecs[x * 2 + 2].iov_len = sizeof(struct ether_header) + 7 + offset;
		memset(ptr, 0, sizeof(struct ether_header) + 7 + offset);
		header = (struct ether_header *) ptr;
		if (!vlan)
			header->ether_type = htons(0x5500);
		else
			header->ether_type = htons(0x8100);
		set_address(header);
		if (vlan) {
			ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (id >> 8);
			ptr[sizeof(struct ether_header) + 1] = id & 0xFF;
			ptr[sizeof(struct ether_header) + 2] = htons(0x5500) & 0xFF;
			ptr[sizeof(struct ether_header) + 3] = htons(0x5500) >> 8;
		}
		ptr[sizeof(struct ether_header) + 1 + offset] = x >> 8;
		ptr[sizeof(struct ether_header) + offset] = x & 0xFF;
		ptr[sizeof(struct ether_header) + 3 + offset] = cols >> 8;
		ptr[sizeof(struct ether_header) + 4 + offset] = cols & 0xFF;
		ptr[sizeof(struct ether_header) + 5 + offset] = 0x08;
		ptr[sizeof(struct ether_header) + 6 + offset] = 0x88;
		iovecs[x * 2 + 3].iov_base = (buffer + (x * cols));
		iovecs[x * 2 + 3].iov_len = cols * sizeof(Matrix_RGB_t);
		msgs[x].msg_hdr.msg_iov = &iovecs[x * 2 + 2];
		msgs[x].msg_hdr.msg_iovlen = 2;
	}
	
	if (sendmmsg(fd, msgs, rows + 2, 0) != rows + 2)
		throw errno;
}
