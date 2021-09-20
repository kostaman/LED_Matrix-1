/* 
 * File:   main.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
using std::cerr;
using std::endl;

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

const int FPS = 60;

int main(int argc, char **argv) {
	int f;
	volatile uint8_t *ptr;
	
	if (argc != 2) {
		cerr << "Usage: sudo ./Matrix \"interface\"" << endl;
		return -1;
	}
	
	Matrix m(argv[1]);
	
	if (daemon(0, 0) < 0)
		throw errno;
	
	if ((f = open("/tmp/LED_Matrix.mem", O_RDWR)) < 0)
		throw errno;
		
	ptr = (volatile uint8_t *) mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
		
	while (1) {
		switch (*ptr) {
			case 0:
				usleep(1000000 / FPS);
				break;
			case 1:
				m.send_frame();
				*ptr = 0;
				break;
			case 2:
				m.send_frame((uint16_t) (*(ptr + 1) << 8 | *(ptr + 2)));
				*ptr = 0;
				break;
			default:
				*ptr = 0;
				break;
		}
	}
	
	return 0;
}
