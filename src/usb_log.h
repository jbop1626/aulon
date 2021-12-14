/*
    usb_log.h
    functions for writing USB communications to a log file

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
#ifndef AULON_USB_LOG_H
#define AULON_USB_LOG_H

void usb_log_set_path(char * path);
void usb_log_start(void);
void usb_log_stop(void);
void usb_log_comms(unsigned char * buffer, unsigned int length, int direction);
void usb_log_error(const char * error);

#endif
