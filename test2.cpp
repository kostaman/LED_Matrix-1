/* 
 * File:   test2.cpp
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on October 9, 2021
 *
 * FOR USE WITH COLORLIGHT HARDWARE
 *
 * This example maps the output of LEDVISION.
 *  LEDVISION does not support the required mapping, however pixel data is available.
 *   (May also be a limitation of my understanding or receiver card firmware.)
 *   One virtual machine runs LEDVISION which outputs to a VNET which is connected to a
 *    Linux VM running the LED_Matrix daemon.
 *   Thus allowing usage of LEDVISION on the display via this application.
 *   Performance and stability of this are not fully known.
 *    This is not expected to work well due to blocking on send_frame. Working around this
 *     requires threads, double buffering, etc. I am not sure how worth this is.
 *    However this may work somewhat for certain things. 
 */

#include <linux/if_packet.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>
#include <algorithm>
#include "C++/Matrix.h"

int main(int argc, char *argv[]) {
	int fd;
	struct ifreq if_idx = {0};
	struct sockaddr_ll sock_addr = {0};
	struct packet_mreq mreq = {0};
	uint8_t buf[1536];
	Matrix *m = new Matrix();
	struct ether_header *eh = (struct ether_header *) buf;
	
	if ((fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
		throw errno;
	
	strcpy(if_idx.ifr_name, "ens39");
	if (ioctl(fd, SIOCGIFINDEX, &if_idx) < 0)
		throw errno;
	
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_ifindex = if_idx.ifr_ifindex;
	sock_addr.sll_protocol = htons(ETH_P_ALL);
	mreq.mr_ifindex = if_idx.ifr_ifindex;
	mreq.mr_type = PACKET_MR_PROMISC;
	
	if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
		throw errno;

	if (bind(fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) 
		throw errno;
		
	m->set_brightness(100);

	while (1) {
		recv(fd, buf, 1536, 0);
		switch (ntohs(eh->ether_type)) {
			case 0x5500:
				if (buf[14] < m->get_rows()) {
					int cols = std::min(m->get_columns(), (uint32_t) buf[18]);
					for (int i = 0; i < cols; i++) {
						Matrix_RGB_t pixel(buf[23 + i * 3], buf[22 + i * 3], buf[21 + i * 3]);
						m->set_pixel(i, buf[14], pixel);
					}
				}
				break;
			case 0x0107:
				m->send_frame();
				break;
			default:
				break;
		}
		
	}

	return 0;
}
