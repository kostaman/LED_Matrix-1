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
	
	// Reducing max size to simplify protocol. (CPU cannot keep up anyhow.)
	//	Max per 5A-75E receiver is 512x256
	//	Max per 5A-75B receiver is 256x256
	//	Max per Ethernet chain is 512x1280
	//	Max per S2 sender is 1024x1280
	rows = r % (max_rows + 1);
	cols = c % (max_cols + 1);
	send_command(Resize_CMD, rows, cols);
	
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
 
void PIC32MZ_NetCard::send_frame(bool vlan, uint16_t vlan_id) {
	const uint32_t num_buffers = (rows * cols) % sizeof(RGB_Packet_t::buffer) ? ((rows * cols) % sizeof(RGB_Packet_t::buffer)) + 1 : (rows * cols) % sizeof(RGB_Packet_t::buffer);
	const uint32_t num_threads = std::min(THREADS, num_buffers);
	uint16_t i = 0;
	Matrix_RGB_t *ptr = mm;
	thread t[num_threads];
	RGB_Packet_t buffer[num_buffers];
	libusb_context *ctx = NULL;
	libusb_device_handle *handle;
	
	send_command(VLAN_CMD, vlan ? 1 : 0, vlan_id);
	for (uint32_t p = rows * cols; p > 0; p -= std::min(p, (uint32_t) sizeof(RGB_Packet_t::buffer))) {
		buffer[i].index = i;
		memcpy(buffer[i].buffer, ptr + (i * sizeof(RGB_Packet_t::buffer)), std::min(p, (uint32_t) sizeof(RGB_Packet_t::buffer)));
		i++;
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

void PIC32MZ_NetCard::set_brightness(uint8_t brightness) {
	send_command(Brightness_CMD, brightness, 0);
}

void PIC32MZ_NetCard::send_command(uint16_t cmd, uint16_t arg1, uint16_t arg2) {
	RGB_Packet_t buffer;
	uint8_t *ptr = (uint8_t *) buffer.buffer;
	libusb_context *ctx = NULL;
	libusb_device_handle *handle;
	
	switch(cmd) {
		case Brightness_CMD:
			ptr[0] = arg1 & 0xFF;
			break;
		case VLAN_CMD:
			ptr[0] = arg1 & 0xFF;
			ptr[1] = arg2 & 0xFF;
			ptr[2] = arg2 >> 8;
			break;
		case Resize_CMD:
			ptr[0] = arg1 & 0xFF;
			ptr[1] = arg1 >> 8;
			ptr[2] = arg2 & 0xFF;
			ptr[3] = arg2 >> 8;
			break;
		default:
			return;
	};
	
	libusb_init(&ctx);
	handle = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle)
		return;

	buffer.index = cmd;
	if (libusb_claim_interface(handle, 0) >= 0)
		worker(handle, &buffer);

	libusb_close(handle);
	libusb_exit(ctx);
}
 
