/* 
 * File:   Matrix.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 29, 2021
 */

#ifndef Matrix_H
#define Matrix_H

#include <stdint.h>

struct Matrix_RGB_t {
	Matrix_RGB_t() : red(0), green(0), blue(0) { }
	Matrix_RGB_t(uint8_t r, uint8_t g, uint8_t b) {
		red = r;
		green = g;
		blue = b;
	}

	uint8_t blue;
	uint8_t green;
	uint8_t red;
};

class Matrix {
	public:
		Matrix(uint8_t channel = 0, uint32_t r = 16, uint32_t c = 128);
		Matrix(char *address, uint16_t p, uint32_t r = 16, uint32_t c = 128);
		~Matrix();
			
		uint32_t get_rows();
		uint32_t get_columns();	
		void send_frame();
		//void send_frame(uint16_t vlan_id);
		void set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
		void set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len);
		void fill(Matrix_RGB_t pixel);
		void clear();
		void set_brightness(uint8_t brightness);
	
	protected: 
		virtual void map_pixel(uint32_t *x, uint32_t *y);
		void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
		void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t *pixels, uint8_t len);
	
		int fd;
		char *address;
		uint16_t port;
		volatile uint8_t *ptr;
		uint32_t rows;
		uint32_t cols;
		uint32_t virt_rows;
		uint32_t virt_cols;
		bool isNet;
		
		const uint32_t marker = 0x09202021;
	
	private:
		struct packet {
			uint32_t marker;
			uint8_t command;
			uint8_t size;
		};
	
		void open_socket();
		void transfer(int client, bool out, void *ptr, uint32_t len);
};

#endif	/* Matrix_H */
