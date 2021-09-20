/* 
 * File:   network.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 20, 2021
 *
 * Warning: This logic is very flawed!
 */

#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

//#include <thread>
//using std::thread;

#include "Matrix.h"
using LED_Matrix::Matrix;
using LED_Matrix::Matrix_RGB_t;

struct packet {
	uint32_t marker;
	uint8_t command;
	uint8_t size;
};

void func(Matrix *m, int client, uint32_t rows, uint32_t cols) {
	packet p;
	uint64_t val;
	uint16_t id;
	struct timeval tv;
	
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		goto exit;
	
	while (1) {
		if (recv(client, &p, sizeof(p), 0) != sizeof(p))
			goto exit;
		
		if (p.marker != 0x09202021) 
			goto exit;
		
		switch (p.command) {
			case 0: // send_frame with VLAN
				if (p.size == sizeof(id)) {
					if (recv(client, &id, sizeof(id), 0) != sizeof(id)) 
						goto exit;
					m->send_frame(id);
				}
				else goto exit;
				break;
			case 1: // send_frame
				if (!p.size) 
					m->send_frame(id);
				else goto exit;
				break;
			case 2: // get rows and cols
				val = ((uint64_t) rows << 32) + cols;
				if (send(client, &val, sizeof(val), 0) != sizeof(val))
					goto exit;
				break;
			case 3: // set_pixel
				if ((p.size - sizeof(uint32_t)) % sizeof(Matrix_RGB_t)) 
					goto exit;
				else {
					uint32_t r, c;
					uint32_t *addr;
					Matrix_RGB_t *pixels;
					uint8_t *ptr = (uint8_t *) malloc(p.size);
					if (ptr == NULL)
						goto exit;
						
					if (recv(client, ptr, p.size, 0) != p.size)
						goto exit;
						
					addr = (uint32_t *) ptr;
					pixels = (Matrix_RGB_t *) (ptr + sizeof(uint32_t));
					for (uint32_t i = 0; i < (p.size - sizeof(uint32_t)) / sizeof(Matrix_RGB_t); i++) {
						r = ((*addr + i) / cols) % rows;
						c = (*addr + i) % cols;
						m->set_pixel_raw(c, r, *(pixels + i));
					}
				}
				break;
			case 4: // clear
				m->clear();
				break;
			default:
				goto exit;
		}
	}
exit:
	close(client);
}

void network(Matrix *m, uint32_t rows, uint32_t cols) {
	int server, handle;
	socklen_t len;
	struct sockaddr_in sock, client;
	
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0)
		return;
		
	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = htonl(INADDR_ANY);
	sock.sin_port = htons(8080);
	
	if (bind(server, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		close(server);
		return;
	}
		
	if (listen(server, 5)  < 0) {
		close(server);
		return;
	}
	
	while (1) {
		handle = accept(server, (struct sockaddr *) &client, &len);
		if (handle < 0) {
			close(server);
			return;
		}
		
		//new thread(func, m, handle, rows, cols);
		func(m, handle, rows, cols);
	}
}
