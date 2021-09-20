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

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

int main(int argc, char **argv) {
	int f;
	volatile uint8_t *ptr;
	
	Matrix m("ens33");
	
	if (daemon(0, 0) < 0)
		throw errno;
	
	if ((f = open("/tmp/LED_Matrix.mem", O_RDWR)) < 0)
		throw errno;
		
	ptr = (volatile uint8_t *) mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
		
	while (1) {
		switch (*ptr) {
			case 1:
				m.send_frame();
				*ptr = 0;
				break;
			case 2:
				m.send_frame((uint16_t) (*(ptr + 1) << 8 | *(ptr + 2)));
				*ptr = 0;
				break;
			default:
				usleep(1000000 / 60);
		}
	}
	
	return 0;
}
