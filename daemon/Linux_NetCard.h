/* 
 * File:   Linux_NetCard.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */

#ifndef LINUX_NETCARD_H
#define LINUX_NETCARD_H

#include <stdint.h>
#include "NetCard.h"

namespace Matrix {    
	class Linux_NetCard : public NetCard {
		public:
			Linux_NetCard(const char *interface, uint32_t channel = 0, uint32_t rows = 64, uint32_t cols = 32);
			
			void send_frame(bool vlan, uint16_t vlan_id);
			void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
			void fill(Matrix_RGB_t pixel);
			void clear();
			void set_brightness(uint8_t brightness);

		protected:			
			int fd;
			uint8_t b_raw;
			uint8_t brightness;
			uint32_t rows;
			uint32_t cols;
			Matrix_RGB_t *buffer;

		private:		
			const uint32_t max_rows = 512;
			const uint32_t max_cols = 1280;
			const uint32_t cols_per_pkt = 497;
	};
}

#endif	/* LINUX_NETCARD_H */

