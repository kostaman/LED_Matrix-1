/* 
 * File:   Matrix.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 29, 2021
 */
 
 #include <sys/mman.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #include <errno.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #include <string.h>
 #include <stdio.h>
 #include "Matrix.h"
 
 Matrix::Matrix(uint32_t channel, uint32_t r, uint32_t c) : virt_rows(r), virt_cols(c) {
 	struct stat st;
	char filename[25];
 	
 	isNet = false;
 	snprintf(filename, 25, "/tmp/LED_Matrix-%d.mem", channel);
 	if ((fd = open(filename, O_RDWR)) < 0)
		throw errno;
	
	fstat(fd, &st);
	ptr = (volatile uint8_t *) mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
		throw errno;
		
	*ptr = 3;
	while(*ptr);
	rows = (*(ptr + 2) << 8) + *(ptr + 3);
		
	*ptr = 4;
	while(*ptr);
	cols = (*(ptr + 2) << 8) + *(ptr + 3);
	
	if (rows * cols < virt_rows * virt_cols)
		throw rows * cols; 
 }
 
 Matrix::Matrix(char *addr, uint16_t pn, uint32_t r, uint32_t c) : port(pn), virt_rows(r), virt_cols(c) {
 	uint32_t data[2];
 	packet p;
 	p.command = 2;
 	p.marker = marker;
 	p.size = 0;
 
	address = addr;
	isNet = true;
	
	open_socket();
	transfer(fd, true, &p, sizeof(p));
	transfer(fd, false, data, 8);
	close(fd);
	
	rows = data[1];
	cols = data[0];
 }
 
 Matrix::~Matrix() {
 	struct stat st;
 	
 	if (isNet)
 		close(fd);
 	else {
 		fstat(fd, &st);
 		close(fd);
 		munmap((void *) ptr, st.st_size);
 	}
 }
 
 uint32_t Matrix::get_rows() {
 	return virt_rows;
 }
 
 uint32_t Matrix::get_columns() {
 	return virt_cols;
 }
 
 void Matrix::send_frame() {
 	packet p;
 
 	if (!isNet) {
 		*ptr = 1;
 		while(*ptr);
 	}
 	else {
 		p.command = 1;
 		p.marker = marker;
 		p.size = 0;
 		open_socket();
 		transfer(fd, true, &p, sizeof(p));
 		close(fd);
 	}
 }
 
/*void Matrix::send_frame(uint16_t vlan_id) {
 	packet p;
 	uint8_t b = 1;
 	
 	if (!isNet) {
 		*(ptr + 1) = vlan_id >> 8;
 		*(ptr + 2) = vlan_id;
 		*(ptr + 3) = 1;
 		*ptr = 2;
 		while(*ptr);
 	}
 	else {
 		p.command = 0;
 		p.marker = marker;
 		p.size = 3;
 		open_socket();
 		transfer(fd, true, &p, sizeof(p));
 		transfer(fd, true, &vlan_id, 2);
 		transfer(fd, true, &b, 1);
 		close(fd);
 	}
 
}*/
 
void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	map_pixel(&x, &y);
	set_pixel_raw(x, y, pixel);
}

void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len) {
	map_pixel(&x, &y);
	set_pixel_raw(x, y, pixels, len);
}

void Matrix::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	packet p;
	Matrix_RGB_t *buffer = (Matrix_RGB_t *) (ptr + 4);
	uint32_t val = (y * cols) + x;
	
	if (!isNet)
		*(buffer + val) = pixel;
	else {
		p.command = 3;
		p.marker = marker;
		p.size = 7;
		open_socket();
		transfer(fd, true, &p, sizeof(p));
		transfer(fd, true, &val, sizeof(val));
		transfer(fd, true, &pixel, 3);
		close(fd);
	}
}

void Matrix::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len) {
	packet p;
	uint32_t val = y * cols + x;
	
	if (!isNet) {
		for (int i = 0; i < len; i++) {
			x = val % cols;
			y = val / cols;
			set_pixel_raw(x, y, pixels[i]);
			val++;
		}
	}
	else {
		if (len > 83)
			throw len;
		p.command = 3;
		p.marker = marker;
		p.size = len * 3 + 4;
		open_socket();
		transfer(fd, true, &p, sizeof(p));
		transfer(fd, true, &val, sizeof(val));
		transfer(fd, true, pixels, len * 3);
		close(fd);
	}
}
		
void Matrix::fill(Matrix_RGB_t pixel) {
	packet p;
	
	if (!isNet) {
		*(ptr + 1) = pixel.red;
		*(ptr + 2) = pixel.green;
		*(ptr + 3) = pixel.blue;
		*ptr = 6;
		while(*ptr);
	}
	else  {
		p.command = 5;
		p.marker = marker;
		p.size = sizeof(pixel);
		open_socket();
		transfer(fd, true, &p, sizeof(p));
		transfer(fd, true, &pixel, p.size);
		close(fd);
	}
}

void Matrix::clear() {
	Matrix_RGB_t p;
	fill(p);
}
		
void Matrix::set_brightness(uint8_t brightness) {
	packet p;

	if (!isNet) {
		*(ptr + 1) = brightness;
		*ptr = 5;
		while(*ptr);
	}
	else {
		p.command = 6;
		p.marker = marker;
		p.size = 1;
		open_socket();
		transfer(fd, true, &p, sizeof(p));
		transfer(fd, true, &brightness, p.size);
		close(fd);
	}
}

inline void Matrix::map_pixel(uint32_t *x, uint32_t *y) {
	uint32_t x2 = *x % cols;
	uint32_t y2 = (*y % virt_rows);
	switch (*x / cols) {
		case 0:
			y2 += virt_rows * 3;
			break;
		case 1:
			y2 += virt_rows * 2;
			break;
		case 2:
			y2 += virt_rows;
			break;
		default:
			break;
	}
	*x = x2;
	*y = y2;
}

void Matrix::open_socket() {
	struct sockaddr_in serv_addr; 
	packet p;
	
 	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		throw errno;

	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port); 

	if (inet_pton(AF_INET, address, &serv_addr.sin_addr) <= 0)
		throw errno;

	if (connect(fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		throw errno;
}

void Matrix::transfer(int client, bool out, void *ptr, uint32_t len) {
	int result;
	if (out)
		result = send(client, ptr, len, 0);
	else
		result = recv(client, ptr, len, 0);
	if (result != len)
		throw result;
}
 
