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
#include <stdio.h>
#include <algorithm>
#include <cmath>
#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

Matrix::Matrix(const char *iface, uint32_t channel, uint32_t r, uint32_t c) {
	int f;
	uint32_t *ptr;
	char filename[25];
	struct ifreq if_idx;
	struct sockaddr_ll sock_addr;
	unsigned dhost[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
	struct mq_attr attr;
	
	// Reducing max size to simplify protocol. (CPU cannot keep up anyhow.)
	//	Max per 5A-75E receiver is 512x256
	//	Max per 5A-75B receiver is 256x256
	//	Max per Ethernet chain is 512x1280
	//	Max per S2 sender is 1024x1280
	rows = r % (max_rows + 1);
	cols = c % (max_cols + 1);
	
	brightness = 0xFF;
	b_raw = 0xFF;
	
 	snprintf(filename, 25, "/tmp/LED_Matrix-%d.mem", channel);
	if ((f = open(filename, O_CREAT | O_RDWR, 0666)) < 0)
		if ((f = open(filename, O_RDWR, 0666)) < 0)
			throw errno;
	
	if (chmod(filename, 0666) < 0)
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

// TODO: Reduce duplication of effort
void Matrix::send_frame(bool vlan, uint16_t vlan_id) {
	uint32_t offset = 0;
	uint32_t pkts_per_row = cols % cols_per_pkt ? (cols / cols_per_pkt) + 1 : cols / cols_per_pkt;
	struct mmsghdr msgs[(rows * pkts_per_row) + 2];
	struct iovec iovecs[(2 * rows * pkts_per_row) + 2];
	struct ether_header *header;
	unsigned char *ptr;
	int x;
	
	vlan_id &= 0xFFF;
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
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (vlan_id >> 8);
		ptr[sizeof(struct ether_header) + 1] = vlan_id & 0xFF;
		ptr[sizeof(struct ether_header) + 2] = htons(0x0107) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0107) >> 8;
	}
	
	ptr[sizeof(struct ether_header) + 21 + offset] = b_raw;		// Global brightness? (Not used)
	ptr[sizeof(struct ether_header) + 22 + offset] = 0x05;		// Enables brightness settings? (Unstable if not set)
	// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
	ptr[sizeof(struct ether_header) + 24 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	ptr[sizeof(struct ether_header) + 25 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	ptr[sizeof(struct ether_header) + 26 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	msgs[rows * pkts_per_row].msg_hdr.msg_iov = &iovecs[0];
	msgs[rows * pkts_per_row].msg_hdr.msg_iovlen = 1;
	
	ptr = (unsigned char *) malloc(77 + offset);
	iovecs[1].iov_base = ptr;
	iovecs[1].iov_len = 77 + offset;
	memset(ptr, 0, 77 + offset);
	header = (struct ether_header *) ptr;
	if (!vlan)
		// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
		header->ether_type = htons(0x0A00 + brightness);
	else
		header->ether_type = htons(0x8100);
	set_address(header);
	if (vlan) {
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (vlan_id >> 8);
		ptr[sizeof(struct ether_header) + 1] = vlan_id & 0xFF;
		// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
		ptr[sizeof(struct ether_header) + 2] = htons(0x0A00 + brightness) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0A00 + brightness) >> 8;
	}
	// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
	ptr[sizeof(struct ether_header) + offset] = brightness;	
	ptr[sizeof(struct ether_header) + 1 + offset] = brightness;
	ptr[sizeof(struct ether_header) + 2 + offset] = 0xFF;		// Function unknown
	msgs[(rows * pkts_per_row) + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[(rows * pkts_per_row) + 1].msg_hdr.msg_iovlen = 1;
	
	for (x = 0; x < rows; x++) {
		uint32_t i = 0;
		for (uint32_t y = cols; y > 0; y -= std::min(cols_per_pkt, y)) {
			ptr = (unsigned char *) malloc(sizeof(struct ether_header) + 7 + offset);
			iovecs[(x * 2 * pkts_per_row) + 2 + (2 * i)].iov_base = ptr;
			iovecs[(x * 2 * pkts_per_row) + 2 + (2 * i)].iov_len = sizeof(struct ether_header) + 7 + offset;
			memset(ptr, 0, sizeof(struct ether_header) + 7 + offset);
			header = (struct ether_header *) ptr;
			if (!vlan)
				header->ether_type = htons(0x5500 + (x >> 8));
			else
				header->ether_type = htons(0x8100);
			set_address(header);
			if (vlan) {
				ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (vlan_id >> 8);
				ptr[sizeof(struct ether_header) + 1] = vlan_id & 0xFF;
				ptr[sizeof(struct ether_header) + 2] = htons(0x5500 + (x >> 8)) & 0xFF;
				ptr[sizeof(struct ether_header) + 3] = htons(0x5500) >> 8;
			}
			ptr[sizeof(struct ether_header) + offset] = x & 0xFF;
			ptr[sizeof(struct ether_header) + 1 + offset] = (cols - y) >> 8;
			ptr[sizeof(struct ether_header) + 2 + offset] = (cols - y) & 0xFF;
			ptr[sizeof(struct ether_header) + 3 + offset] = std::min(cols_per_pkt, y) >> 8;
			ptr[sizeof(struct ether_header) + 4 + offset] = std::min(cols_per_pkt, y) & 0xFF;
			ptr[sizeof(struct ether_header) + 5 + offset] = 0x08;	// Function unknown
			ptr[sizeof(struct ether_header) + 6 + offset] = 0x88;	// Function unknown
			iovecs[(x * 2 * pkts_per_row) + 3 + (2 * i)].iov_base = (buffer + (x * cols) + cols - y);
			iovecs[(x * 2 * pkts_per_row) + 3 + (2 * i)].iov_len = std::min(cols_per_pkt, y) * sizeof(Matrix_RGB_t);
			msgs[(x * pkts_per_row) + i].msg_hdr.msg_iov = &iovecs[(x * 2 * pkts_per_row) + (2 * i) + 2];
			msgs[(x * pkts_per_row) + i].msg_hdr.msg_iovlen = 2;
			i++;
		}
	}	
	
	for (int i = 0; i < pkts_per_row; ++i)
		if (sendmmsg(fd, &msgs[i * rows], rows, 0) != rows)
			throw errno;
	if (sendmmsg(fd, &msgs[pkts_per_row * rows], 2, 0) != 2)
		throw errno;
		
	for (x = 0; x < rows; x++) {
		uint32_t i = 0;
		for (uint32_t y = cols; y > 0; y -= std::min(cols_per_pkt, y)) {
			free(iovecs[(x * 2 * pkts_per_row) + 2 + (2 * i)].iov_base);
			i++;
		}
   	}
}
