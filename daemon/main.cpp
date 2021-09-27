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
#include <string.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <thread>
using std::thread;

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

const int FPS = 60;

extern void network(Matrix *m, uint32_t rows, uint32_t cols);

void usage(int e, char **argv) {
	if (e != 0)
		cerr << "Error: " << strerror(e) << endl << endl;
	cerr << "Usage: sudo " << argv[0] << " \"interface\" <rows> <cols>" << endl;
	exit(-1);
}

int main(int argc, char **argv) {
	int f;
	uint16_t rows, cols;
	volatile uint8_t *ptr;
	Matrix *m;
	
	if (argc != 4)
		usage(0, argv);
	
	rows = atoi(argv[2]);
	cols = atoi(argv[3]);
	
	try {
		m = new Matrix(argv[1], rows, cols);
	} 
	catch (int e) {
		usage(e, argv);
	}
	
	if (daemon(0, 0) < 0)
		throw errno;
		
	thread t(network, m, rows, cols);
	
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
				m->send_frame();
				*ptr = 0;
				break;
			case 2:
				m->send_frame((uint16_t) (*(ptr + 1) << 8 | *(ptr + 2)));
				*ptr = 0;
				break;
			case 3:
				*(ptr + 2) = rows >> 8;
				*(ptr + 3) = rows & 0xFF;
				*ptr = 0;
				break;
			case 4:
				*(ptr + 2) = cols >> 8;
				*(ptr + 3) = cols & 0xFF;
				*ptr = 0;
				break;
			case 5:
				m->set_brightness(*(ptr + 1));
				*ptr = 0;
				break;
			case 6:
				m->fill(Matrix_RGB_t(*(ptr + 1), *(ptr + 2), *(ptr + 3)));
				*ptr = 0;
				break;
			default:
				*ptr = 0;
				break;
		}
	}
	
	return 0;
}
