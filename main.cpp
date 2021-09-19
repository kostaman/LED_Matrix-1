/* 
 * File:   main.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */
 
#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

int main(int argc, char **argv) {
	uint8_t x = 0;
	
	Matrix m("ens33");
	
	while(1) {
		m.fill(Matrix_RGB_t(x, x, x));
		m.send_frame((uint16_t) 0x12);
		x++;
	}
	
	return 0;
}
