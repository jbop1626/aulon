/*
    io.c
    general input/output utilities

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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "io.h"

static int is_end_of_buffer_line(unsigned int i, unsigned int length) {
    int not_first_element = (i != 0);
    int not_end_of_buffer = (i != length - 1);
    int full_line_printed = ((i + 1) % 0x10 == 0);
    return (not_first_element) && (not_end_of_buffer) && (full_line_printed);
}


void print_buffer(unsigned char * buffer, unsigned int length, FILE * const outstream) {
    unsigned int i;
    for (i = 0; i < length; ++i) {
        fprintf(outstream, "%02x ", buffer[i]);
        if (is_end_of_buffer_line(i, length)) {
            fprintf(outstream, "\n");
        }
    }
    fprintf(outstream, "\n");
}

int get_input(char * line_buffer, int buffer_length, FILE * instream) {
    char * result = fgets(line_buffer, buffer_length, instream);
    if (result == NULL || ferror(instream)) {
        fprintf(stderr, "ERROR: Could not read input!\n");
        line_buffer[0] = '\0';
        return 0;
    }

    // If fgets returned after hitting limit, discard extra characters in instream.
    // Otherwise it returned after receiving a newline; replace it with null.
    result = (char *)memchr(line_buffer, '\n', buffer_length);
    if (result == NULL) {
        while (getc(instream) != '\n');
    }
    else {
        result[0] = '\0';
    }

    return 1;
}

int open_file(FILE ** file, const char * filename, const char * mode) {
    if (file == NULL || filename == NULL || mode == NULL) {
        fprintf(stderr, "ERROR: NULL argument(s) provided to open_file(). Opening file aborted.\n");
        return 0;
    }
    if(strlen(filename) > FILENAME_MAX) {
        fprintf(stderr, "ERROR: Filename is too long. Opening file aborted.\n");
        return 0;
    }

    errno = 0;
    *file = fopen(filename, mode);
    if (*file == NULL) {
        perror("Error opening file for reading");
        return 0;
    }
    else {
        return 1;
    }
}

size_t get_file_size(FILE * file) {
    unsigned char * buffer = calloc(0x4000, sizeof(unsigned char));
    rewind(file);
    
    size_t count = 0;
    while (1) {
        size_t read_count = fread(buffer, sizeof(buffer[0]), 0x4000, file);
        count += read_count;
        if (ferror(file)) {
            fprintf(stderr, "Error calculating file size!\n");
            count = 0;
            break;
        }
        if (feof(file)) {
            break;
        }
    }
    
    rewind(file);
    free(buffer);
    return count;
}

int file_size_check(FILE * file, size_t expected_size) {
    return (get_file_size(file) == expected_size);
}

// Assumes 8-bit char and array of 4 bytes as input
uint32_t uchars_to_uint32(unsigned char * bytes) {
    uint32_t out = 0;
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        out <<= 8;
        out |= bytes[i];
    }
    return out;
}

// Assumes 8-bit char and array of 4 bytes as input
int32_t uchars_to_int32(unsigned char * bytes) {
    int32_t out = 0;
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        out <<= 8;
        out |= bytes[i];
    }
    return out;
}

// Assumes 8-bit char and array of 2 bytes as input
int16_t uchars_to_int16(unsigned char * bytes) {
    int16_t out = 0;
    unsigned int i;
    for (i = 0; i < 2; ++i) {
        out <<= 8;
        out |= bytes[i];
    }
    return out;
}
