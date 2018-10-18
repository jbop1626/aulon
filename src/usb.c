/*
	usb.c
	low-level USB communications and transfers

	Copyright (c) 2018 Jbop (https://github.com/jbop1626)
	Copyright (c) 2012-2018 Mike Ryan

	This file is a part of aulon.

	aulon is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	aulon is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	Portions by Mike Ryan were originally released under the MIT License
	as contained in LICENSES/MIT.txt.
*/

#include <libusb.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "usb.h"

static const uint16_t IQUE_VID = 0x1527; // 0xBB3D for old test SAs that support USB
static const uint16_t IQUE_PID = 0xBBDB;
static const unsigned char IQUE_BULK_EP_OUT = 0x02;
static const unsigned char IQUE_BULK_EP_IN  = 0x82;

static struct libusb_device_handle * device_handle = NULL;
static int cleanup_required = 0;
static int interface_claimed = 0;
static int usb_initialized = 0;


static void usb_init(void) {
    if (libusb_init(NULL) < 0) {
        fprintf(stderr, "libusb could not be initialized; exiting...\n");
		fflush(stderr);
        exit(EXIT_FAILURE);
    }
	usb_initialized = 1;
}


static int usb_get_device_handle(uint16_t vendor_id, uint16_t product_id) {
    device_handle = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
    return usb_handle_exists();
}


static int usb_connect_to_device(void) {
	if (!cleanup_required) {
		atexit(usb_close_connection);
	}

	if (!usb_get_device_handle(IQUE_VID, IQUE_PID)) {
		fprintf(stderr, "The device could not be opened. Make sure it is plugged in!\n");
		return 0;
	}
	
    int r = libusb_claim_interface(device_handle, 0);
    if (r < 0) {
        fprintf(stderr, "libusb_claim_interface error: %s\n", libusb_error_name(r));
        return 0;
    }

	cleanup_required = 1;
	interface_claimed = 1;
    return 1;
}


int usb_init_connection(void) {
	usb_init();
	return usb_connect_to_device();
}


int usb_close_connection(void) {
	if (interface_claimed) {
        int r = libusb_release_interface(device_handle, 0);
		if (r < 0) {
			fprintf(stderr, "libusb_release_interface error: %s\n", libusb_error_name(r));
			return 0;
		}
	}
	if (device_handle) {
		libusb_close(device_handle);
	}
	if (usb_initialized) {
		libusb_exit(NULL);
		usb_initialized = 0;
	}

	device_handle = NULL;
	interface_claimed = 0;
	return 1;
}


int usb_handle_exists(void) {
	return device_handle != NULL;
}


int usb_bulk_transfer_send(unsigned char * data, int length, int * actual_length, unsigned int timeout) {
	int r = libusb_bulk_transfer(device_handle, IQUE_BULK_EP_OUT, data, length, actual_length, timeout);
	if (r < 0) {
		fprintf(stderr, "SEND - libusb_bulk_transfer error: %s\n", libusb_error_name(r));
		return 0;
	}
	return 1;
}


int usb_bulk_transfer_receive(unsigned char * data, int length, int * actual_length, unsigned int timeout) {
	int r = libusb_bulk_transfer(device_handle, IQUE_BULK_EP_IN, data, length, actual_length, timeout);
	if (r < 0) {
		fprintf(stderr, "RECEIVE - libusb_bulk_transfer error: %s\n", libusb_error_name(r));
		return 0;
	}
	return 1;
}

