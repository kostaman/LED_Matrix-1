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

#include <cmath>
#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

uint32_t Matrix::queue_num = 0;

Matrix::Matrix(const char *iface, uint32_t channel, uint32_t r, uint32_t c, bool db) : doubleBuffer(db) {
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
	rows = r % 512;
	cols = c % 497;
	
	brightness = 0xFF;
	b_raw = 0xFF;
	
	if (doubleBuffer) {
		attr.mq_flags = 0;
		attr.mq_maxmsg = 5;
		attr.mq_msgsize = sizeof(Queue_MSG);
		attr.mq_curmsgs = 0;
		snprintf(filename, 25, "%s-%d", queue_name, get_queue_num());
		if ((queue = mq_open(filename, O_RDWR | O_CREAT, 0666, &attr)) == -1)
			throw -1;
		
		stop = false;
		if (pthread_mutex_init(&b_lock, NULL) != 0)
			throw errno;
		if (pthread_mutex_init(&q_lock, NULL) != 0)
			throw errno;
		if (pthread_create(&thread, NULL, (void *(*)(void *)) &Matrix::send_frame_thread, this))
			throw errno;
	}
	
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

Matrix::~Matrix() {
	void *result;
	if (doubleBuffer) {
		stop = true;
		pthread_join(thread, &result);
		if (mq_close(queue) != -1)
			mq_unlink(queue_name);
		pthread_mutex_destroy(&b_lock);
		pthread_mutex_destroy(&q_lock);
	}
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
	if (doubleBuffer)
		pthread_mutex_lock(&b_lock);
	b_raw = round(b / 100.0 * 255.0);
	brightness = round(pow(b / 100.0, 0.405) * 255.0);
	if (doubleBuffer)
		pthread_mutex_unlock(&b_lock);
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

void Matrix::send_frame(bool vlan, uint16_t id) {
	if (doubleBuffer) {
		Matrix_RGB_t *buf = new Matrix_RGB_t[rows * cols];
		Queue_MSG frame = {vlan, id, buf};
		memcpy(buf, buffer, sizeof(Matrix_RGB_t) * rows * cols);
		mq_send(queue, (char *) &frame, sizeof(Queue_MSG), 0);	// frame is memcpy into queue by mq_send
	}
	else {
		Queue_MSG frame = {vlan, id, buffer};
		send_frame_pkts(frame);
	}
}

void Matrix::send_frame_pkts(Queue_MSG frame) {
	uint32_t offset = 0;
	struct mmsghdr msgs[rows + 2];
	struct iovec iovecs[(2 * rows) + 2];
	struct ether_header *header;
	unsigned char *ptr;
	int x;
	
	frame.vlan_id &= 0xFFF;
	if (frame.vlan)
		offset = 4;
	
	memset(msgs, 0, sizeof(msgs));
	memset(iovecs, 0, sizeof(iovecs));
	
	ptr = (unsigned char *) malloc(112 + offset);
	iovecs[0].iov_base = ptr;
	iovecs[0].iov_len = 112 + offset;
	memset(ptr, 0, 112 + offset);
	header = (struct ether_header *) ptr;
	if (!frame.vlan)
		header->ether_type = htons(0x0107);
	else
		header->ether_type = htons(0x8100);
	set_address(header);
	if (frame.vlan) {
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (frame.vlan_id >> 8);
		ptr[sizeof(struct ether_header) + 1] = frame.vlan_id & 0xFF;
		ptr[sizeof(struct ether_header) + 2] = htons(0x0107) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0107) >> 8;
	}
	
	if (doubleBuffer)
		pthread_mutex_lock(&b_lock);
	
	ptr[sizeof(struct ether_header) + 21 + offset] = b_raw;		// Global brightness? (Not used)
	ptr[sizeof(struct ether_header) + 22 + offset] = 0x05;		// Enables brightness settings? (Unstable if not set)
	// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
	ptr[sizeof(struct ether_header) + 24 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	ptr[sizeof(struct ether_header) + 25 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	ptr[sizeof(struct ether_header) + 26 + offset] = b_raw;		// R, B or G brightness ? (Not used)
	msgs[rows].msg_hdr.msg_iov = &iovecs[0];
	msgs[rows].msg_hdr.msg_iovlen = 1;
	
	ptr = (unsigned char *) malloc(77 + offset);
	iovecs[1].iov_base = ptr;
	iovecs[1].iov_len = 77 + offset;
	memset(ptr, 0, 77 + offset);
	header = (struct ether_header *) ptr;
	if (!frame.vlan)
		// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
		header->ether_type = htons(0x0A00 + brightness);
	else
		header->ether_type = htons(0x8100);
	set_address(header);
	if (frame.vlan) {
		ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (frame.vlan_id >> 8);
		ptr[sizeof(struct ether_header) + 1] = frame.vlan_id & 0xFF;
		// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
		ptr[sizeof(struct ether_header) + 2] = htons(0x0A00 + brightness) & 0xFF;
		ptr[sizeof(struct ether_header) + 3] = htons(0x0A00 + brightness) >> 8;
	}
	// Changes with Color Temperature setting (Not used), bell curve shift of R and/or B centered at 6500 (default)
	ptr[sizeof(struct ether_header) + offset] = brightness;	
	ptr[sizeof(struct ether_header) + 1 + offset] = brightness;
	ptr[sizeof(struct ether_header) + 2 + offset] = 0xFF;		// Function unknown
	msgs[rows + 1].msg_hdr.msg_iov = &iovecs[1];
	msgs[rows + 1].msg_hdr.msg_iovlen = 1;
	
	if (doubleBuffer)
		pthread_mutex_unlock(&b_lock);
	
	for (x = 0; x < rows; x++) {
		ptr = (unsigned char *) malloc(sizeof(struct ether_header) + 7 + offset);
		iovecs[x * 2 + 2].iov_base = ptr;
		iovecs[x * 2 + 2].iov_len = sizeof(struct ether_header) + 7 + offset;
		memset(ptr, 0, sizeof(struct ether_header) + 7 + offset);
		header = (struct ether_header *) ptr;
		if (!frame.vlan)
			header->ether_type = htons(0x5500 + (x >> 8));
		else
			header->ether_type = htons(0x8100);
		set_address(header);
		if (frame.vlan) {
			ptr[sizeof(struct ether_header) + 0] = (0xE << 4) | (frame.vlan_id >> 8);
			ptr[sizeof(struct ether_header) + 1] = frame.vlan_id & 0xFF;
			ptr[sizeof(struct ether_header) + 2] = htons(0x5500 + (x >> 8)) & 0xFF;
			ptr[sizeof(struct ether_header) + 3] = htons(0x5500) >> 8;
		}
		ptr[sizeof(struct ether_header) + offset] = x & 0xFF;
		ptr[sizeof(struct ether_header) + 3 + offset] = cols >> 8;
		ptr[sizeof(struct ether_header) + 4 + offset] = cols & 0xFF;
		ptr[sizeof(struct ether_header) + 5 + offset] = 0x08;	// Function unknown
		ptr[sizeof(struct ether_header) + 6 + offset] = 0x88;	// Function unknown
		iovecs[x * 2 + 3].iov_base = (frame.buffer + (x * cols));
		iovecs[x * 2 + 3].iov_len = cols * sizeof(Matrix_RGB_t);
		msgs[x].msg_hdr.msg_iov = &iovecs[x * 2 + 2];
		msgs[x].msg_hdr.msg_iovlen = 2;
	}
	
	if (sendmmsg(fd, msgs, rows + 2, 0) != rows + 2)
		throw errno;
}

void *Matrix::send_frame_thread(void *arg) {
	struct timespec t;
	Queue_MSG msg;
	Matrix *m = (Matrix *) arg;
	
	while (!m->stop) {
		clock_gettime(CLOCK_REALTIME, &t);
		t.tv_sec += 1;
		if (mq_timedreceive(m->queue, (char *) &msg, sizeof(Queue_MSG), NULL, &t) > 0) {
			m->send_frame_pkts(msg);
			delete msg.buffer;
		}
	}
	
	return 0;
}

uint32_t Matrix::get_queue_num() {
	uint32_t result;
	pthread_mutex_lock(&q_lock);
	result = queue_num;
	++queue_num;
	pthread_mutex_unlock(&q_lock);
	return result;
}
