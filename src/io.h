/*
    io.h

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
#ifndef AULON_IO_H
#define AULON_IO_H

#include <stdio.h>
#include <stdint.h>

void print_buffer(unsigned char * buffer, unsigned int length, FILE * const outstream);
int get_input(char * line_buffer, int buffer_length, FILE * instream);
int open_file(FILE ** file, const char * filename, const char * mode);
size_t get_file_size(FILE * file);
int file_size_check(FILE * file, size_t expected_size);
uint32_t uchars_to_uint32(unsigned char * bytes);
int32_t uchars_to_int32(unsigned char * bytes);
int16_t uchars_to_int16(unsigned char * bytes);

#endif
