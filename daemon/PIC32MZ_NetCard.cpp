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
#include <thread>
using Matrix::PIC32MZ_NetCard;
using Matrix::Matrix_RGB_t;
using std::thread;
 
PIC32MZ_NetCard::PIC32MZ_NetCard(uint32_t channel) {
	int f;	
	uint32_t *ptr;
	char filename[25];

	for (int i = 0; i < (ROW / SCALER); i++)
		buffer[i].index = i;
	
	snprintf(filename, 25, "/tmp/LED_Matrix-%d.mem", channel);
	if ((f = open(filename, O_CREAT | O_RDWR, 0666)) < 0)
		if ((f = open(filename, O_RDWR, 0666)) < 0)
			throw errno;
	
	if (chmod(filename, 0666) < 0)
		throw errno;
		
	if (ftruncate(f, 4 + (256 * ROW * sizeof(Matrix_RGB_t))) < 0)
		throw errno;
	
	ptr = (uint32_t *) mmap(NULL, 4 + (256 * ROW * sizeof(Matrix_RGB_t)), PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
	
	mm = (Matrix_RGB_t *) (ptr + 1);
}
 
void PIC32MZ_NetCard::send_frame(bool, uint16_t) {
	Matrix_RGB_t *ptr = mm;
	thread t[THREADS];
	libusb_context *ctx = NULL;
	libusb_device_handle *handle;
	
	for (int i = 0; i < (ROW / SCALER); i++) {
		memcpy(buffer[i].buffer, ptr, 256 * SCALER * sizeof(Matrix_RGB_t));
		ptr += 256 * SCALER;
	}
	
	libusb_init(&ctx);
	handle = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle)
		return;
	
	if (libusb_claim_interface(handle, 0) >= 0) {	
		for (int x = 0; x < (ROW / SCALER); x += THREADS) {
			for (int i = 0; i < THREADS; i++)
				t[i] = thread(worker, handle, &buffer[x + i]);
			for (int i = 0; i < THREADS; i++)
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
	x %= ROW;
	y %= 256;
	buffer[x / SCALER].buffer[((x % SCALER) * 256) + y] = pixel;
}
		
void PIC32MZ_NetCard::fill(Matrix_RGB_t pixel) {	
	for (int x = 0; x < ROW; x++)
		for (int y = 0; y < 256; y++)
			set_pixel_raw(x, y, pixel);
}

void PIC32MZ_NetCard::clear() {
	Matrix_RGB_t p;
	fill(p);
}
 
