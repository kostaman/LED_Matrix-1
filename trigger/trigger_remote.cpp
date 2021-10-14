/* 
 * File:   trigger_remote.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 29, 2021
 */
 
#include "../C++/Matrix.h"

int main(int argc, char **argv) {

	{
		Matrix *m = new Matrix();
		Matrix_RGB_t p(255, 255, 255);
		m->set_pixel(0, 0, p);
		m->send_frame();
		//m->send_frame(13);
		m->fill(p);
		m->send_frame();
	}

	{
		Matrix *m = new Matrix("127.0.0.1");
		Matrix_RGB_t p(255, 255, 255);
		m->set_pixel(0, 0, p);
		m->send_frame();
		//m->send_frame(13);
		m->fill(p);
		m->send_frame();
	}

	return 0;
}
