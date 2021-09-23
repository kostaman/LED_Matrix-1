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
			Matrix(const char *interface, uint32_t rows = 64, uint32_t cols = 32);
			
			virtual uint32_t get_rows() { return 16; }
			virtual uint32_t get_columns() { return 128; }
			
			void send_frame();
			void send_frame(uint16_t vlan_id);
			
			void set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
			void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
			void fill(Matrix_RGB_t pixel);
			void clear();
			void set_brightness(uint8_t brightness);

		protected:
			virtual void map(uint32_t *x, uint32_t *y);
			
			int fd;
			uint8_t b_raw, brightness;
			uint32_t rows;
			uint32_t cols;
			Matrix_RGB_t *buffer;

		private:
			void send_frame(bool vlan, uint16_t vlan_id);
	};
}

#endif	/* Matrix_H */
