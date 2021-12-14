/*
    commands.h

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
#ifndef AULON_COMMANDS_H
#define AULON_COMMANDS_H

#include <stdint.h>

// USB command numbers
enum {
    WRITE_BLOCK_ONLY        = 0x06,
    READ_BLOCK_ONLY         = 0x07,
//  UNKNOWN_COMMAND         = 0x0D, // Scan bad blocks?; needs more testing
    WRITE_BLOCK_AND_SPARE   = 0x10,
    READ_BLOCK_AND_SPARE    = 0x11,
    INIT_FS                 = 0x12,
    GET_NUM_BLOCKS          = 0x15,
    SET_SEQNO               = 0x16,
    GET_SEQNO               = 0x17,
    FILE_CHKSUM             = 0x1C,
    SET_LED                 = 0x1D,
    SET_TIME                = 0x1E,
    GET_BBID                = 0x1F,
    SIGN_HASH               = 0x20
};

enum { 
    BLOCK_SIZE       = 0x4000,
    BLOCK_CHUNK_SIZE = 0x1000,
    CHUNKS_PER_BLOCK = 4,
    NUM_BLOCKS       = 0x1000,
    SPARE_SIZE       = 0x10
};

enum {
    SHA1_HASH_LENGTH = 0x14,
    ECC_SIG_LENGTH   = 0x40
};

int write_block_only(unsigned char * block_buffer, uint32_t block_number);
int read_block_only(unsigned char * block_buffer, uint32_t block_number);
int write_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number);
int read_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number);
int init_fs(void);
int get_num_blocks(void);
int set_seqno(uint32_t arg);
int get_seqno(void);
int file_checksum_cmp(const char * filename, uint32_t checksum, uint32_t size);
int set_led(uint32_t arg);
int set_time(uint32_t first_half, unsigned char * second_half);
int get_bbid(uint32_t * bbid_out);
int sign_hash(unsigned char * hash_in, unsigned char * sig_out);

#endif
