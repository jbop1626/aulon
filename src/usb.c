/*
    usb.c
    low-level USB communications and transfers

    Copyright (c) 2018,2019,2020 Jbop (https://github.com/jbop1626)
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include "defs.h"
#include "usb.h"
#include "usb_log.h"


static const uint16_t IQUE_VID = 0x1527; // 0xBB3D for old test SAs that support USB
static const uint16_t IQUE_PID = 0xBBDB;
static const unsigned char IQUE_BULK_EP_OUT = 0x02;
static const unsigned char IQUE_BULK_EP_IN  = 0x82;

static struct libusb_device_handle * device_handle = NULL;
static int cleanup_required  = 0;
static int kernel_detached   = 0;
static int interface_claimed = 0;
static int usb_initialized   = 0;


static void usb_cleanup_close(void) {
    usb_close_connection();
}


static void usb_init(void) {
    if (libusb_init(NULL) < 0) {
        fprintf(stderr, "libusb could not be initialized; exiting...\n");
        exit(EXIT_FAILURE);
    }
    usb_initialized = 1;
}


static int usb_get_device_handle(uint16_t vendor_id, uint16_t product_id) {
    device_handle = libusb_open_device_with_vid_pid(NULL, vendor_id, product_id);
    return usb_handle_exists();
}


static int usb_detach_kernel_driver(void) {
#ifdef __linux__ // Only need the following on Linux
    int r = libusb_kernel_driver_active(device_handle, 0);
    if (r == 1) {
        if (libusb_detach_kernel_driver(device_handle, 0) < 0) {
            fprintf(stderr, "libusb_detach_kernel_driver error: %s\n", libusb_error_name(r));
            return 0;
        }
        kernel_detached = 1;
    }
    else if (r < 0) {
        fprintf(stderr, "libusb_kernel_driver_active error: %s\n", libusb_error_name(r));
        return 0;
    }
#endif
    return 1;
}


static int usb_check_and_set_device_configuration(int desired_config) {
    int active_config = -1;
    
    int r = libusb_get_configuration(device_handle, &active_config);
    if (r < 0) {
        fprintf(stderr, "libusb_get_configuration error: %s\n", libusb_error_name(r));
        return 0;
    }
    
    if (active_config != desired_config) {
#ifdef __linux__ // Only need the following on Linux
//      r = libusb_reset_device(device_handle);
//      if (r < 0) {
//            fprintf(stderr, "libusb_reset_device error: %s, exiting...\n", libusb_error_name(r));
//            exit(EXIT_FAILURE);
//      }
#endif
        r = libusb_set_configuration(device_handle, desired_config);
        if (r < 0) {
            fprintf(stderr, "libusb_set_configuration error: %s\n", libusb_error_name(r));
            return 0;
        }
    }
    
    return 1;
}


static int usb_claim_device_interface(void) {
    // Make sure the device is not in an unconfigured state before claiming interface
    int r = usb_check_and_set_device_configuration(1);
    if (r < 0) {
        fprintf(stderr, "libusb_claim_interface error: %s\n", libusb_error_name(r));
        return 0;
    }
    
    r = libusb_claim_interface(device_handle, 0);
    if (r < 0) {
        fprintf(stderr, "libusb_claim_interface error: %s\n", libusb_error_name(r));
        return 0;
    }
    interface_claimed = 1;
    
    // Check (and possibly set) again to be sure the configuration wasn't changed in the meantime
    r = usb_check_and_set_device_configuration(1);
    if (r < 0) {
        fprintf(stderr, "libusb_claim_interface error: %s\n", libusb_error_name(r));
        return 0;
    }
    
    return 1;
}


static int usb_connect_to_device(void) {
    if (!cleanup_required) {
        cleanup_required = 1;
        atexit(usb_cleanup_close);
    }

    if (!usb_get_device_handle(IQUE_VID, IQUE_PID)) {
        fprintf(stderr, "The device could not be opened. Make sure it is plugged in!\n");
        return 0;
    }
    
    if (!usb_detach_kernel_driver()) {
        fprintf(stderr, "Could not detach kernel driver.\n");
        return 0;
    }

    if (!usb_claim_device_interface()) {
        fprintf(stderr, "Error configuring device connection.\n");
        return 0;
    }
#if defined(AULON_LOGGING_ENABLED) && (AULON_LOGGING_ENABLED == 1)
    usb_log_start();
#endif
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
        interface_claimed = 0;
    }
    if (kernel_detached) {
        int r = libusb_attach_kernel_driver(device_handle, 0);
        if (r < 0) {
            fprintf(stderr, "libusb_attach_kernel_driver error: %s\n", libusb_error_name(r));
            return 0;
        }
        kernel_detached = 0;
    }
    if (device_handle) {
        libusb_close(device_handle);
        device_handle = NULL;
    }
    if (usb_initialized) {
        libusb_exit(NULL);
        usb_initialized = 0;
    }
#if defined(AULON_LOGGING_ENABLED) && (AULON_LOGGING_ENABLED == 1)
    usb_log_stop();
#endif
    return 1;
}


int usb_handle_exists(void) {
    return (device_handle != NULL);
}


static int handle_usb_error(int error_code, unsigned char endpoint, int length, int * actual_length, unsigned int timeout) {
    int success = 0; 
    const char * direction = (endpoint == IQUE_BULK_EP_IN ? "RECEIVE" : "SEND");
    
    switch(error_code) {
        case LIBUSB_ERROR_TIMEOUT:
            // fprintf(stderr, "\nUSB connection timed out; %u bytes of data were transferred.\n", *actual_length);
            return (*actual_length != 0);
        case LIBUSB_ERROR_PIPE:
            libusb_clear_halt(device_handle, endpoint);
            break;
        case LIBUSB_ERROR_INTERRUPTED:
            break;
        default: // Unrecoverable error
            fprintf(stderr, "\n%s - libusb_bulk_transfer FATAL error: %s\n%s\n\n", direction, libusb_error_name(error_code), libusb_strerror(error_code));
            fprintf(stderr, "If this error occurred while WRITING blocks or files to the player,\nDO NOT POWER OFF OR RESET YOUR CONSOLE!\n");
            fprintf(stderr, "Instead, simply attempt the write operation again.\n");
            fprintf(stderr, "Alternatively, restart aulon, or use ique_diag.exe, in order to continue or restart the writing operation.\n");
            exit(EXIT_FAILURE);
            break;
    }
    
    fprintf(stderr, "%s - libusb_bulk_transfer error: %s (length: %d, actual_length: %d, timeout: %u)\n",
            direction, libusb_error_name(error_code), length, *actual_length, timeout);
    return success;
}

int usb_bulk_transfer_send(unsigned char * data, int length, int * actual_length, unsigned int timeout) {
    int success = 1;
    int r = libusb_bulk_transfer(device_handle, IQUE_BULK_EP_OUT, data, length, actual_length, timeout);
    if (r < 0) {
        success = handle_usb_error(r, IQUE_BULK_EP_OUT, length, actual_length, timeout);
    }
#if defined(AULON_LOGGING_ENABLED) && (AULON_LOGGING_ENABLED == 1)
    if (success) {
        usb_log_comms(data, *actual_length, 1);
    }
    else {
        usb_log_error(libusb_error_name(r));
    }
#endif
    return success;
}


int usb_bulk_transfer_receive(unsigned char * data, int length, int * actual_length, unsigned int timeout) {
    int success = 1;
    int r = libusb_bulk_transfer(device_handle, IQUE_BULK_EP_IN, data, length, actual_length, timeout);
    if (r < 0) {
        success = handle_usb_error(r, IQUE_BULK_EP_IN, length, actual_length, timeout);
    }
#if defined(AULON_LOGGING_ENABLED) && (AULON_LOGGING_ENABLED == 1)
    if (success) {
        usb_log_comms(data, *actual_length, 0);
    }
    else {
        usb_log_error(libusb_error_name(r));
    }
#endif
    return success;
}
