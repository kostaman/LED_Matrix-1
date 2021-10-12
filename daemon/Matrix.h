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
#include <mqueue.h>
#include <pthread.h>

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
			Matrix(const char *interface, uint32_t rows = 64, uint32_t cols = 32, bool doubleBuffer = false);
			~Matrix();
			
			void send_frame();
			void send_frame(uint16_t vlan_id);
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
			struct Queue_MSG {
				bool vlan;
				uint16_t vlan_id;
				Matrix_RGB_t *buffer;
			};
		
			pthread_t thread;
			mqd_t queue;
			bool stop;
			bool doubleBuffer;
			const char *queue_name = "/queue";
			
			void send_frame_pkts(Queue_MSG frame);
			void send_frame(bool vlan, uint16_t vlan_id);
			static void *send_frame_thread(void *arg);
	};
}

#endif	/* Matrix_H */
