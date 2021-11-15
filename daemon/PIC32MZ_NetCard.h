/* 
 * File:   PIC32MZ_NetCard.h
 * Author: David Thacher
 * License: GPL 3.0
 *
 * Created on September 29, 2021
 */

#ifndef PIC32MZ_NETCARD_H
#define PIC32MZ_NETCARD_H

#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include "NetCard.h"

namespace Matrix {
	class PIC32MZ_NetCard : public NetCard {
		public:
			PIC32MZ_NetCard(uint32_t channel);			// TODO: Allow optimizing memory usage, USB bandwidth and performance with rows and columns

			void send_frame(bool vlan, uint16_t vlan_id);
			void set_pixel_raw(uint32_t x, uint32_t y, Matrix_RGB_t pixel);
			void fill(Matrix_RGB_t pixel);
			void clear();
			void set_brightness(uint8_t brightness) { }

		protected: 
			const static uint16_t ROW = 256;			// Max value of 512. Lower to improve performance.
			const static uint16_t USB_VENDOR_ID = 0x04d8;		// Do not change without updating firmware.
			const static uint16_t USB_PRODUCT_ID = 0xffff;	// Do not change without updating firmware.
			const static uint32_t USB_TIMEOUT = 3000;
			const static uint16_t THREADS = 16;			// Do not change unless working on system like a VM to increase performance.
			const static uint8_t SCALER = 2;			// Do not change without updating/reworking firmware.

			struct RGB_Packet_t {
				uint16_t index;
				Matrix_RGB_t buffer[256 * SCALER];
			};

			RGB_Packet_t buffer[ROW / SCALER];
			Matrix_RGB_t *mm;

		private:
			static void worker(libusb_device_handle *handle, RGB_Packet_t *buffer);
	};
}

#endif	/* PIC32MZ_NETCARD_H */

