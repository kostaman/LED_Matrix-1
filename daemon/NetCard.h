/* 
 * File:   NetCard.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on November 15, 2021
 */

#ifndef NETCARD_H
#define NETCARD_H

#include <stdint.h>

namespace Matrix {
	struct Matrix_RGB_t {
		uint8_t blue;
		uint8_t green;
		uint8_t red;
		
		Matrix_RGB_t() {}
		Matrix_RGB_t(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {}
	} __attribute__((packed));
    
	class NetCard {
		public:
			virtual void send_frame(bool vlan, uint16_t vlan_id) = 0;
			virtual void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel) = 0;
			virtual void fill(Matrix_RGB_t pixel) = 0;;
			virtual void clear() = 0;
			virtual void set_brightness(uint8_t brightness) = 0;
	};
}

#endif	/* NETCARD_H */

