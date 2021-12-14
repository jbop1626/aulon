/*
    usb_log.c
    functions for writing USB communications to a log file

    Copyright (c) 2018,2020 Jbop (https://github.com/jbop1626)
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
#include <stdio.h>

#include "usb_log.h"
#include "io.h"


static int log_open = 0;
static char * log_path = NULL;
static FILE * log_file = NULL;


void usb_log_set_path(char * path) {
    log_path = path;
}

void usb_log_start(void) {
    if (!log_path) {
        // Log file path wasn't specified, so just exit.
        return;
    }
    if (!open_file(&log_file, log_path, "a")) {
        fprintf(stderr, "Log file could not be opened. Logging aborted.\n");
        return;
    }
    fprintf(log_file, "\nLogging session started:\n");
    log_open = 1;
}

void usb_log_stop(void) {
    if(log_open) {
        fprintf(log_file, "Logging session ended.\n");
        fclose(log_file);
        log_open = 0;
    }
}

void usb_log_comms(unsigned char * buffer, unsigned int length, int direction) {
    if (log_open) {
        char * direction_label = (direction ? "SEND >>>" : "RECEIVE <<<");
        fprintf(log_file, "%s %d bytes:\n", direction_label, length);
        print_buffer(buffer, length, log_file);
        fflush(log_file);
    }
}

void usb_log_error(const char * error) {
    if (log_open) {
        fprintf(log_file, "\nERROR - %s\n\n", error);
        fflush(log_file);
    }
}
