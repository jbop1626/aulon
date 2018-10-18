/*
    operations.h

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

#ifndef AULON_OPERATIONS_H
#define AULON_OPERATIONS_H

#include <stdint.h>

enum { 
    BLOCK_SIZE       = 0x4000,
    BLOCK_CHUNK_SIZE = 0x1000,
    CHUNKS_PER_BLOCK = 4,
    NUM_BLOCKS       = 0x1000,
    SPARE_SIZE       = 0x10
};

/*
    I/O
*/
void print_buffer(unsigned char * buffer, unsigned int length);
int open_file(FILE ** file, const char * filename, const char * mode);
int dump_nand_and_spare_to_files(FILE * nand_file, FILE * spare_file);

/*
    COMMANDS
*/
int read_block_only(unsigned char * block_buffer, uint32_t block_number);
int read_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number);
int get_bbid(uint32_t * bbid_out);

#endif

