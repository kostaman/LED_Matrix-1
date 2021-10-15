/* 
 * File:   test3.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on October 15, 2021
 *
 * Warning this needs work.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include "C++/Matrix.h"
#include "font8x8_basic.h"

class Mapper : public Matrix {
	public:
		Mapper(uint32_t channel = 0, uint32_t r = 32, uint32_t c = 128) : Matrix(channel, r, c) { }
	
	protected:
		void map_pixel(uint32_t *x, uint32_t *y) {
			uint32_t x2 = *x % cols;
			uint32_t y2 = (*y % virt_rows);
			switch (*x / cols) {
				case 0:
					y2 += virt_rows;
				default:
					break;
			}
			*x = x2;
			*y = y2;
		}
};

const int flag = 0;

char* time_str(const struct tm *timeptr) {
	static const char wday_name[][4] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char mon_name[][4] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static const char hr[][3] = {
	  	"AM", "PM"
	};
	static char result[33];
	snprintf(result, 33, "  %3s %2d:%.2d %2s    %3s %2d, %4d  ",
		wday_name[timeptr->tm_wday],
		(timeptr->tm_hour % 12) ? timeptr->tm_hour % 12 : (timeptr->tm_hour % 12) + 12,
		timeptr->tm_min,
		hr[timeptr->tm_hour / 12],
		mon_name[timeptr->tm_mon],
		timeptr->tm_mday, 
		1900 + timeptr->tm_year);
	return result;
}

int main(int argc, char **argv) {
	Matrix *m = new Mapper(1);
	const int rows = m->get_rows();
	const int cols = m->get_columns();
	Matrix_RGB_t buffer[rows][cols];
	
	int index = 0;
	int counter = 0;
	char str[] = "Hello World Testing 1 2 3";
	
	int len = strlen(str);
	char *s;
	
	if (len % 16)
		len += 16 - len % 16;
	s = (char *) malloc(len + 1);
	memset(s, ' ', len);
	s[len] = 0;
	strcpy(s, str);
	
	m->set_brightness(10);
	
	while (1) {
		if (flag) {
			for (int i = 1; i < rows; i++)
				memcpy(&buffer[i - 1][0], &buffer[i][0], sizeof(Matrix_RGB_t) * cols);
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < cols / 8; j++) {
					Matrix_RGB_t pixel;
					if (font8x8_basic[s[j + index]][counter] & 1 << i) {
						pixel.red = (char) 255;
						pixel.green = (char) 128;
					}
					buffer[15][j * 8 + i] = pixel;
				}
			}
			if (++counter >= 8) {
				counter = 0;
				index += cols / 8;
				if (index >= len)
					index = 0;
			}
			usleep(100 * 1000);
		}
		else {
			usleep(500 * 1000);
			time_t t;
			time(&t);
			char *s = time_str(localtime(&t));
			m->clear();
			for (int x = 0; x < cols; x++) {
				for (int y = 0; y < rows; y++) {
					Matrix_RGB_t pixel;
					if (font8x8_basic[s[y / 8 * 16 + x / 8]][y % 8] & 1 << x % 8) {
						if (y / 8)
							pixel.green = (char) 255;
						else
							pixel.red = (char) 255;
					}
					buffer[y][x] = pixel;
				}
			}
		}
		for (int x = 0; x < cols; x++)
			for (int y = 0; y < rows; y++)
				m->set_pixel(x, y, buffer[y][x]);
		m->send_frame();
	}

	return 0;
}
