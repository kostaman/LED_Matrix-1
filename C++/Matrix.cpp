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
 
 Matrix::Matrix() {
 	struct stat st;
 	
 	isNet = false;
 	if ((fd = open("/tmp/LED_Matrix.mem", O_RDWR)) < 0)
		throw errno;
	
	fstat(fd, &st);
	ptr = (volatile uint8_t *) mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
		throw errno;
		
	*ptr = 3;
	while(*ptr);
	rows = *(ptr + 2) << 8 + *(ptr + 3);
		
	*ptr = 4;
	while(*ptr);
	cols = *(ptr + 2) << 8 + *(ptr + 3);
 }
 
 Matrix::Matrix(char *addr) {
 	uint32_t data[2];
 	packet p;
 	p.command = 2;
 	p.marker = marker;
 	p.size = 0;
 
	address = addr;
	isNet = true;
	
	connect();
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
 		unmap((void *) ptr, st.st_size);
 	}
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
 		transfer(fd, true, &p, sizeof(p));
 	}
 }
 
 void Matrix::send_frame(uint16_t vlan_id) {
 	packet p;
 	
 	if (!isNet) {
 		*(ptr + 1) = vlan_id >> 8;
 		*(ptr + 2) = vlan_id;
 		*ptr = 2;
 		while(*ptr);
 	}
 	else {
 		p.command = 0;
 		p.marker = marker;
 		p.size = 2;
 		transfer(fd, true, &p, sizeof(p));
 		transfer(fd, true, &vlan_id, p.size);
 	}
 
 }
 
void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {

}

void Matrix::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len) {

}

void Matrix::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {

}

void Matrix::set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len) {

}
		
void Matrix::fill(Matrix_RGB_t pixel) {

}

void Matrix::clear() {
	Matrix_RGB_t p();
	fill(p);
}
		
void Matrix::set_brightness(uint8_t brightness) {

}

inline void Matrix::map_pixel(uint32_t *x, uint32_t *y) {
	uint32_t x2 = *x % cols;
	uint32_t y2 = (*x / cols * 16) + (*y % 16);
	*x = x2;
	*y = y2;
}

void Matrix::connect() {
	struct sockaddr_in serv_addr; 
	packet p;
	
 	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		throw errno;

	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8080); 

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
 
