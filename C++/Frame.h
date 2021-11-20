/* 
 * File:   Frame.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on November 19, 2021
 */

#ifndef Frame_H
#define Frame_H

#include <stdint.h>
#include <vector>
#include "Matrix.h"
using std::vector;

class Frame {
	public:
		Frame(uint32_t r = 16, uint32_t c = 128);
		// TODO: Add copy constructor
 		// TODO: Add destructor
			
		uint32_t get_rows();
		uint32_t get_columns();
		
		// Note: Use threads with caution, disabled by default
		//  Threads allows mappers and send_frame of Matrix class to run in parallel
		void send_frame(uint8_t threads = 0);
		
		// TODO: Improve these - consider thread safe logic
		// Note: Graphics API is provided by something like OpenGL, QT, Java libs, etc.
		void set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
		void fill(Matrix_RGB_t pixel);
		void clear();
		
		void map_channel(Matrix *m, uint32_t x, uint32_t y);
	
	protected: 
		uint32_t rows;
		uint32_t cols;
		
		// TODO: Consider improving this
		Matrix_RGB_t *buffer;
	
	private:		
		struct MatrixChannel {
			MatrixChannel(Matrix *p_m, uint32_t start_x, uint32_t start_y) {
				m = p_m;
				x = start_x;
				y = start_y;
			}
			
			Matrix *m;
			uint32_t x;
			uint32_t y;
		};
		
		static void worker(MatrixChannel *c, Matrix_RGB_t *b, uint32_t cols);
		
		vector<MatrixChannel> chans;
};

#endif	/* Frame_H */
