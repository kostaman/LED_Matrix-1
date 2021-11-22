/* 
 * File:   Frame.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on November 19, 2021
 */
 
 #include <algorithm>
 #include <thread>
 #include "Frame.h"
 using std::thread;
 
 Frame::Frame(uint32_t r, uint32_t c) {
 	rows = r;
 	cols = c;
 	buffer = new Matrix_RGB_t[rows * cols];
 }
 
 uint32_t Frame::get_rows() {
 	return rows;
 }
 
 uint32_t Frame::get_columns() {
 	return cols;
 }
 
void Frame::set_pixel(uint32_t x, uint32_t y, Matrix_RGB_t pixel) {
	x %= cols;
	y %= rows;
	*(buffer + (y * cols) + x) = pixel;
}
		
void Frame::fill(Matrix_RGB_t pixel) {
	for (uint32_t x = 0; x < cols; x++)
		for (uint32_t y = 0; y < rows; y++)
			*(buffer + (y * cols) + x) = pixel;
}

void Frame::clear() {
	Matrix_RGB_t p;
	fill(p);
}

void Frame::map_channel(Matrix *m, uint32_t x, uint32_t y) {
	MatrixChannel c(m, x, y);
	chans.push_back(c);
}

void Frame::send_frame(uint8_t threads) {
	threads = std::min(std::max(threads, (uint8_t) 1), (uint8_t) chans.size());
	thread t[threads];

	for (uint32_t i = 0; i < chans.size(); i += threads) {
		for (uint8_t j = 0; j < threads; j++) {
			MatrixChannel *s = &chans[i + j];
			t[j] = thread(worker, &chans[i + j], buffer + (s->y * s->x) + s->x, cols);
		}
		for (uint8_t j = 0; j < threads; j++)
			t[j].join();
	}
}
		
void Frame::worker(MatrixChannel *c, Matrix_RGB_t *b, uint32_t cols) {
	for (uint32_t x = 0; x < c->m->get_columns(); x++)
		for (uint32_t y = 0; y < c->m->get_rows(); y++)
			c->m->set_pixel(x, y, *(b + (y * cols) + x));
	c->m->send_frame();
}
 
