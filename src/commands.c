/*
    commands.c
    implementations of commands sent to the player

    Copyright (c) 2018,2019,2020,2021 Jbop (https://github.com/jbop1626)
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
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "player_comms.h"
#include "commands.h"
#include "io.h"


static int command_error(unsigned char * buffer);

static int request_block_read(uint32_t command, uint32_t block_number);
static int get_block(unsigned char * block_buffer);
static int get_spare(unsigned char * spare_buffer);

static int request_block_write(uint32_t command, uint32_t block_number);
static int check_block_write(uint32_t block_number);
static int send_block(unsigned char * block_buffer);
static int send_spare(unsigned char * spare_buffer);

static int send_filename(const char * filename);
static int send_params_and_receive_reply(uint32_t checksum, uint32_t size);


/*
    Some commands return negative error codes in the second word
    of the command response to indicate an error occured.
*/
static int command_error(unsigned char * buffer) {
    return uchars_to_int32(&buffer[4]) < 0;
}


/*
    read_block
    One command (READ_BLOCK_ONLY) reads a block only. The other
    (READ_BLOCK_AND_SPARE) reads both the block and the spare area
    of the block's last page.
*/
int read_block_only(unsigned char * block_buffer, uint32_t block_number) {
    unsigned attempts = 1;
    int success = 0;
    while(attempts <= 5) {
        attempts++;
        if (!request_block_read(READ_BLOCK_ONLY, block_number)) {
            success = 0;
        }
        else if (!get_block(block_buffer)) {
            fprintf(stderr, "Reading block 0x%04x failed.\n", block_number);
            success = 0;
        }
        else {
            success = 1;
            break;
        }
    }
    if (!success) {
        fprintf(stderr, "Reading block unsuccessful after 5 retries!\n");
    }
    return success;
}

int read_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number) {
    unsigned attempts = 1;
    int success = 0;
    while (attempts <= 5) {
        attempts++;
        if (!request_block_read(READ_BLOCK_AND_SPARE, block_number)) {
            success = 0;
        }
        else if (!get_block(block_buffer)) {
            fprintf(stderr, "Reading block 0x%04x failed.\n", block_number);
            success = 0;
        }
        else if (!get_spare(spare_buffer)) {
            fprintf(stderr, "Reading block 0x%04x spare failed.\n", block_number);
            success = 0;
        }
        else {
            success = 1;
            break;
        }
    }
    if (!success) {
        fprintf(stderr, "Reading block unsuccessful after 5 retries!\n");
    }
    return success;
}

static int request_block_read(uint32_t command, uint32_t block_number) {    
    if (!ique_send_command(command, block_number)) {
        fprintf(stderr, "Command to read block 0x%04x was sent, but it was not received by the console.\n", block_number);
        return 0;
    }
    
    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        // fprintf(stderr, "Console response to command not received.\nAssuming the worst and aborting block read.\n");
        return 0;
    }
    
    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error when requesting to read block 0x%04x.\n", block_number);
        return 0;
    }
    
    return 1;
}

static int get_block(unsigned char * block_buffer) {
    unsigned char chunk_buffer[BLOCK_CHUNK_SIZE] = { 0 };
    unsigned int i;
    unsigned int offset = 0;
    for (i = 0; i < CHUNKS_PER_BLOCK; ++i) {
        if (ique_receive_reply(chunk_buffer, BLOCK_CHUNK_SIZE)) {
            memcpy(block_buffer + offset, chunk_buffer, BLOCK_CHUNK_SIZE);
            offset += BLOCK_CHUNK_SIZE;
        }
        else {
            return 0;
        }
    }

    return 1;
}

static int get_spare(unsigned char * spare_buffer) {
    return ique_receive_reply(spare_buffer, SPARE_SIZE);
}


/*
    write_block
    One command (WRITE_BLOCK_ONLY) writes a block only. The other
    (WRITE_BLOCK_AND_SPARE) writes both the block and the spare area
    of the block's last page.
*/
int write_block_only(unsigned char * block_buffer, uint32_t block_number) {
    unsigned int attempts = 1;
    int success = 0;
    while (attempts <= 5) {
        attempts++;
        if (!request_block_write(WRITE_BLOCK_ONLY, block_number)) {
            success = 0;
        }
        else if (!send_block(block_buffer)) {
            fprintf(stderr, "Writing block 0x%04x failed.\n", block_number);
            success = 0;
        }
        else if (!check_block_write(block_number)) {
            success = 0;
        }
        else {
            success = 1;
            break;
        }
    }
    if (!success) {
        fprintf(stderr, "Writing block unsuccessful after 5 retries!\n");
    }
    return success;
}

int write_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number) {
    if (spare_buffer[5] != 0xFF) {
        // Block is marked bad; just return normally
        return 1;
    }
    
    unsigned attempts = 1;
    int success = 0;
    while (attempts <= 5) {
        attempts++;
        if (!request_block_write(WRITE_BLOCK_AND_SPARE, block_number)) {
            success = 0;
        }
        else if (!send_block(block_buffer)) {
            fprintf(stderr, "Writing block 0x%04x failed.\n", block_number);
            success = 0;
        }
        else if (!send_spare(spare_buffer)) {
            fprintf(stderr, "Writing block 0x%04x spare failed.\n", block_number);
            success = 0;
        }
        else if (!check_block_write(block_number)) {
            success = 0;
        }
        else {
            success = 1;
            break;
        }
    }
    if (!success) {
        fprintf(stderr, "Writing block unsuccessful after 5 retries!\n");
    }
    return success;
}

static int request_block_write(uint32_t command, uint32_t block_number) {
    if (!ique_send_command(command, block_number)) {
        fprintf(stderr, "Command to write block 0x%04x was sent, but it was not received by the console.\n", block_number);
        return 0;
    }
    ique_wait_for_ready();
    return 1;
}

static int check_block_write(uint32_t block_number) {
    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Error after writing block: Console did not send success state response.\n");
        return 0;
    }

    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error after writing block 0x%04x.\n", block_number);
        return 0;
    }

    return 1;
}

static int send_block(unsigned char * block_buffer) {
    return ique_send_chunked_data(block_buffer, BLOCK_SIZE);
}

static int send_spare(unsigned char * spare_buffer) {
    ique_wait_for_ready();
    // Other than the SA data (first 3 bytes), rest can all be 0xFF.
    unsigned int i;
    for (i = 3; i < SPARE_SIZE; ++i) {
        spare_buffer[i] = 0xFF;
    }
    return ique_send_piecemeal_data(spare_buffer, SPARE_SIZE);
}


/*
    init_fs
    Purpose not entirely known.
    SA1 calls osBbFInit when it receives this command.
*/
int init_fs(void) {
    if (!ique_send_command(INIT_FS, 0x0000)) {
        fprintf(stderr, "INIT_FS command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to INIT_FS command not received.\n");
        return 0;
    }
    
    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error when initializing the FS.\n");
        return 0;
    }
    
    return 1;
}


/*
    get_num_blocks
    Returns the number of blocks in the current iQue Player NAND.
    All known cards have 0x1000 blocks.
*/
int get_num_blocks(void) {
    if (!ique_send_command(GET_NUM_BLOCKS, 0x0000)) {
        fprintf(stderr, "GET_NUM_BLOCKS command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to GET_NUM_BLOCKS command not received.\n");
        return 0;
    }
    
    if (uchars_to_uint32(&reply_buffer[4]) != 0x1000) {
        fprintf(stderr, "The current iQue NAND does not contain exactly 0x1000 blocks.\n");
        fprintf(stderr, "aulon does not currently support cards with such sizes, and will abort.\n");
        return 0;
    }
    
    return 1;
}


/*
    set_seqno
    Purpose unknown.
*/
int set_seqno(uint32_t arg) {
    if (!ique_send_command(SET_SEQNO, arg)) {
        fprintf(stderr, "SET_SEQNO command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to SET_SEQNO command not received.\n");
        return 0;
    }

    return 1;
}


/*
    get_seqno
    Purpose unknown.
*/
int get_seqno(void) {
    if (!ique_send_command(GET_SEQNO, 0x0000)) {
        fprintf(stderr, "GET_SEQNO command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to GET_SEQNO command not received.\n");
        return 0;
    }

    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error in response to GET_SEQNO command.\n");
        return 0;
    }
    
    return 1;
}


/*
    file_checksum_cmp
    Sends a filename, checksum, and file size to the console. The console returns
    zero if the file on the console with the given filename has a matching
    checksum and size, or less-than-zero otherwise.
*/
int file_checksum_cmp(const char * filename, uint32_t checksum, uint32_t size) {
    return send_filename(filename) && send_params_and_receive_reply(checksum, size);
}

static int send_filename(const char * filename) {
    uint32_t fn_len = strlen(filename) + 1;
    if (fn_len > 13) {
        fprintf(stderr, "The given filename is invalid; it is too long for the iQue Player FS.\n");
        return 0;
    }
    
    if (!ique_send_command(FILE_CHKSUM, fn_len)) {
        fprintf(stderr, "FILE_CHKSUM command was sent, but it was not received by the console.\n");
        return 0;
    }
    ique_wait_for_ready();
    
    unsigned char * fn_data = calloc(fn_len, sizeof(unsigned char));
    if (fn_data == NULL) {
        fprintf(stderr, "Could not allocate memory for filename data.\n");
        return 0;
    }
    
    memcpy(fn_data, filename, fn_len - 1);
    
    if (!ique_send_piecemeal_data(fn_data, fn_len)) {
        fprintf(stderr, "Error sending filename to the console.\n");
        free(fn_data);
        return 0;
    }
    free(fn_data);
    ique_wait_for_ready();
    return 1;
}

static int send_params_and_receive_reply(uint32_t checksum, uint32_t size) {
    // Note: Not actually a command, just the same format
    if (!ique_send_command(checksum, size)) {
        fprintf(stderr, "Error sending potential checksum and file size to the console.\n");
        return 0;
    }
    
    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to FILE_CHKSUM command not received.\n");
        return 0;
    }
    
    if (command_error(reply_buffer)) {
        return 0;
    }
    return 1;
}


/*
    set_led
    Causes the LED on the front of the console to light up.
*/
int set_led(uint32_t arg) {
    if (!ique_send_command(SET_LED, arg)) {
        fprintf(stderr, "SET_LED command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to SET_LED command not received.\n");
        return 0;
    }
    
    return 1;
}


/*
    set_time
    Sets the console's clock to the current PC time
*/
int set_time(uint32_t first_half, unsigned char * second_half) {    
    if (!ique_send_command(SET_TIME, first_half)) {
        fprintf(stderr, "SET_TIME command was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to SET_TIME command not received.\n");
        return 0;
    }

    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error when setting the time.\n");
        return 0;
    }

    if(!ique_send_piecemeal_data(second_half, 4)) {
        fprintf(stderr, "Error when sending time data to the console.\n");
        return 0;
    }
    return 1;
}


/*
    get_bbid
    Retrieves the unique id number of the console
*/
int get_bbid(uint32_t * bbid_out) {
    if (!ique_send_command(GET_BBID, 0x0000)) {
        fprintf(stderr, "BBID request was sent, but it was not received by the console.\n");
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to BBID request not received.\n");
        return 0;
    }

    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error when requesting the console's ID.\n");
        return 0;
    }

    // last 4 bytes of the 8-byte reply are the BBID
    *bbid_out = uchars_to_uint32(&reply_buffer[4]);
    return 1;
}


/*
    sign_hash
    Sends an SHA1 hash to the console, which responds with an ECC signature for
    the hash. hash_in assumes a 20-byte SHA1 hash, and sig_out assumes a 64-byte
    ECC signature.
*/
int sign_hash(unsigned char * hash_in, unsigned char * sig_out) {
    if (!ique_send_command(SIGN_HASH, SHA1_HASH_LENGTH)) {
        fprintf(stderr, "Request to sign hash was sent, but it was not received by the console.\n");
        return 0;
    }

    ique_wait_for_ready();
    ique_send_chunked_data(hash_in, SHA1_HASH_LENGTH);

    unsigned char reply_buffer[8] = { 0 };
    if (!ique_receive_reply(reply_buffer, 8)) {
        fprintf(stderr, "Console response to sign_hash request not received.\n");
        return 0;
    }

    if (command_error(reply_buffer)) {
        fprintf(stderr, "The console returned an error in response to sign_hash request.\n");
        return 0;
    }

    if (!ique_receive_reply(sig_out, ECC_SIG_LENGTH)) {
        fprintf(stderr, "ECC signature not received.\n");
        return 0;
    }
    
    return 1;
}

