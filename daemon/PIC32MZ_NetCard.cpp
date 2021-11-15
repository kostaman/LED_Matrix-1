/* 
 * File:   PIC32MZ_NetCard.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 29, 2021
 */


#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "PIC32MZ_NetCard.h"
#include <algorithm>
#include <thread>
using Matrix::PIC32MZ_NetCard;
using Matrix::Matrix_RGB_t;
using std::thread;
 
PIC32MZ_NetCard::PIC32MZ_NetCard(uint32_t channel, uint32_t r, uint32_t c) {
	int f;	
	uint32_t *ptr;
	char filename[25];
	
	rows = r;
	cols = c;
	num_buffers = rows / SCALER;
			
	if (num_buffers > (ROW / SCALER) || cols > 256)
		throw -1;
	
	snprintf(filename, 25, "/tmp/LED_Matrix-%d.mem", channel);
	if ((f = open(filename, O_CREAT | O_RDWR, 0666)) < 0)
		if ((f = open(filename, O_RDWR, 0666)) < 0)
			throw errno;
	
	if (chmod(filename, 0666) < 0)
		throw errno;
		
	if (ftruncate(f, 4 + (rows * cols * sizeof(Matrix_RGB_t))) < 0)
		throw errno;
	
	ptr = (uint32_t *) mmap(NULL, 4 + (rows * cols * sizeof(Matrix_RGB_t)), PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
	
	mm = (Matrix_RGB_t *) (ptr + 1);
}
 
void PIC32MZ_NetCard::send_frame(bool, uint16_t) {
	const uint8_t num_threads = std::min(THREADS, num_buffers);
	Matrix_RGB_t *ptr = mm;
	thread t[num_threads];
	RGB_Packet_t buffer[num_buffers];
	libusb_context *ctx = NULL;
	libusb_device_handle *handle;
	
	for (int i = 0; i < num_buffers; i++) {
		buffer[i].index = i;
		memset(buffer[i].buffer, 0, sizeof(buffer[i].buffer));
		memcpy(buffer[i].buffer, ptr + (i * cols), cols * sizeof(Matrix_RGB_t));
		memcpy(buffer[i].buffer + 256, ptr + (i * cols) + cols, cols * sizeof(Matrix_RGB_t));
	}
	
	libusb_init(&ctx);
	handle = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle)
		return;
	
	if (libusb_claim_interface(handle, 0) >= 0) {	
		for (int x = 0; x < num_buffers; x += num_threads) {
			for (int i = 0; i < num_threads; i++)
				t[i] = thread(worker, handle, &buffer[x + i]);
			for (int i = 0; i < num_threads; i++)
				t[i].join();
		}
	}

	libusb_close(handle);
	libusb_exit(ctx);
}

void PIC32MZ_NetCard::worker(libusb_device_handle *handle, RGB_Packet_t *buffer) {
	int n;
	libusb_bulk_transfer(handle, (LIBUSB_ENDPOINT_OUT | 1), (uint8_t *) buffer, sizeof(RGB_Packet_t), &n, USB_TIMEOUT);
}
 
void PIC32MZ_NetCard::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	y %= rows;
	x %= cols;
    	*(mm + (y * cols) + x) = pixel;
}
		
void PIC32MZ_NetCard::fill(Matrix_RGB_t pixel) {	
	for (int x = 0; x < cols; x++)
		for (int y = 0; y < rows; y++)
			set_pixel_raw(x, y, pixel);
}

void PIC32MZ_NetCard::clear() {
	Matrix_RGB_t p;
	fill(p);
}
 
