/* 
 * File:   main.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 18, 2021
 */

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <iostream>
using std::cerr;
using std::endl;

#include <thread>
using std::thread;

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

const int FPS = 60;

struct channel_cfg {
	string iface;
	uint32_t rows;
	uint32_t cols;
	uint8_t channel;
	uint16_t port;
	bool vlan;
	uint8_t vlan_id;
	bool doubleBuffer;
}

extern void network(Matrix *m, uint32_t rows, uint32_t cols, uint16_t port);
void channel_thread(channel_cfg cfg);

int main(int argc, char **argv) {
	int num_threads = 1;
	thread channels[num_threads];
	channel_cfg cfgs[num_threads];
	
	if (argc != 2) {
		cerr << "Usage: sudo " << argv[0] << "<config_file>" << endl;
		exit(-1);
	}
	
	// TODO: Process configuration file
	cfgs[0].iface = "ens33";
	cfgs[0].rows = 64;
	cfgs[0].cols = 32;
	cfgs[0].channel = 0;
	cfgs[0].port = 8080;
	cfgs[0].vlan = false;
	cfgs[0].vlan_id = 13;
	cfgs[0].doubleBuffer = false;
	
	if (daemon(0, 0) < 0)
		throw errno;
		
	for (int i = 0; i < num_threads; i++)
		channels[i] = thread(channel_thread, cfgs[i]);
	
	for (int i = 0; i < num_threads; i++)
		channels[i].join();
	
	return 0;
}

void channel_thread(channel_cfg cfg) {
	int f;
	char filename[25];
	volatile uint8_t *ptr;
	Matrix *m = new Matrix(cfg.iface, cfg.channel, cfg.rows, cfg.cols, cfg.doubleBuffer);
		
	thread t(network, m, cfg.rows, cfg.cols, cfg.port);
	
	snprintf(filename, 25, "/tmp/LED_Matrix-%d.mem", cfg.channel);
	if ((f = open(filename, O_RDWR)) < 0)
		throw errno;
		
	ptr = (volatile uint8_t *) mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, f, 0);
	if (ptr == MAP_FAILED)
		throw errno;
		
	while (1) {
		switch (*ptr) {
			case 0:
				usleep(1000000 / FPS);
				break;
			case 1:
				m->send_frame(cfg.vlan, cfg.vlan_id);
				*ptr = 0;
				break;
			case 2: // Allows bypass of config - not used
				m->send_frame(*(ptr + 3) != 0, (uint16_t) (*(ptr + 1) << 8 | *(ptr + 2)));
				*ptr = 0;
				break;
			case 3:
				*(ptr + 2) = cfg.rows >> 8;
				*(ptr + 3) = cfg.rows & 0xFF;
				*ptr = 0;
				break;
			case 4:
				*(ptr + 2) = cfg.cols >> 8;
				*(ptr + 3) = cfg.cols & 0xFF;
				*ptr = 0;
				break;
			case 5:
				m->set_brightness(*(ptr + 1));
				*ptr = 0;
				break;
			case 6:
				m->fill(Matrix_RGB_t(*(ptr + 1), *(ptr + 2), *(ptr + 3)));
				*ptr = 0;
				break;
			default:
				*ptr = 0;
				break;
		}
	}
}

