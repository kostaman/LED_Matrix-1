/* 
 * File:   trigger.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 19, 2021
 */

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>

int main(int argc, char **argv) {
	int f;
	volatile uint8_t *ptr;
	
	if ((f = open("/tmp/LED_Matrix.mem", O_RDWR)) < 0)
		throw errno;
		
	ptr = (volatile uint8_t *) mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
	
	// Trigger send_frame
	*ptr = 1;
	
	// Trigger send_frame with VLAN 13
	*(ptr + 2) = 13;
	*ptr = 2;
	
	return 0;
}
