/*
	usb.h

	Copyright (c) 2018 Jbop (https://github.com/jbop1626)
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
*/
#ifndef AULON_USB_H
#define AULON_USB_H

/*
	All functions return 1 for success and 0 for failure.
*/
int usb_init_connection(void);
int usb_close_connection(void);
int usb_handle_exists(void);
int usb_bulk_transfer_send(unsigned char * data, int length, int * actual_length, unsigned int timeout);
int usb_bulk_transfer_receive(unsigned char * data, int length, int * actual_length, unsigned int timeout);

#endif

