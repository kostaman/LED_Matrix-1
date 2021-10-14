/* 
 * File:   trigger_remote.c
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 21, 2021
 */

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h> 

struct packet {
	uint32_t marker;
	uint8_t command;
	uint8_t size;
};

void transfer(int client, uint8_t out, void *ptr, uint32_t len) {
	int result;
	if (out)
		result = send(client, ptr, len, 0);
	else
		result = recv(client, ptr, len, 0);
	if (result != len)
		exit(1);
}

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in serv_addr; 
	struct packet p;
	uint8_t data[8];

	if (argc != 2) {
		printf("Usage: %s <ip of server>\n", argv[0]);
		return 1;
	} 

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return errno;

	memset(&serv_addr, '0', sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8080); 

	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
		return errno;

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		return errno;
    
    	// Set pixel
    	p.command = 3;
    	p.marker = 0x09202021;
    	p.size = 7;
    	memset(data, 0, 7);
    	data[4] = 255;
	transfer(sockfd, 1, &p, sizeof(p));
	transfer(sockfd, 1, data, p.size);
    
    	// Trigger send_frame
    	p.command = 1;
    	p.marker = 0x09202021;
    	p.size = 0;
	transfer(sockfd, 1, &p, sizeof(p));
    
    	// Trigger send_frame with VLAN 13
    	/*p.command = 0;
    	p.marker = 0x09202021;
    	p.size = 2;
    	memset(data, 0, 2);
    	data[0] = 13;
	transfer(sockfd, 1, &p, sizeof(p));
	transfer(sockfd, 1, data, p.size);*/
	
	// Get the number of Rows
    	p.command = 2;
    	p.marker = 0x09202021;
    	p.size = 0;
    	memset(data, 0, 2);
	transfer(sockfd, 1, &p, sizeof(p));
	transfer(sockfd, 0, data, 8);
	
	printf("ROWS: %d, COLS %d\n", (int) (*((uint64_t *) data) >> 32), (int) (*((uint64_t *) data) & 0xFFFFFFFF));
    
	return 0;
}
