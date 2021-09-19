/* 
 * File:   Matrix.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */

#ifndef Matrix_H
#define	Matrix_H

#include <stdint.h>

namespace LED_Matrix {
	struct Matrix_RGB_t {
		uint8_t blue;
		uint8_t green;
		uint8_t red;
		
		Matrix_RGB_t() {}
		Matrix_RGB_t(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
	} __attribute__((packed));
    
	class Matrix {
		public:
			Matrix(const char *interface);
			~Matrix();
			
			void send_frame();
			void send_frame(uint16_t vlan_id);
			
			void set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
			void fill(Matrix_RGB_t pixel);
			void clear();

		protected:
			virtual void map(uint32_t *x, uint32_t *y);
			
			const uint32_t rows = 64;
			const uint32_t cols = 32;
		
			int fd;
			Matrix_RGB_t *buffer;

		private:
	};
}

#endif	/* Matrix_H */

