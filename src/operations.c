/*
	operations.c
	general functions used by menu options, etc.

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

#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <arpa/inet.h>
#endif

#include "player_comms.h"
#include "operations.h"

static int command_error(uint32_t command, unsigned char * buffer);
static int request_block_read(uint32_t command, uint32_t block_number);
static int get_block(unsigned char * block_buffer);
static int get_spare(unsigned char * spare_buffer);

/*
	I/O
*/
void print_buffer(unsigned char * buffer, unsigned int length) {
	unsigned int i;
	for (i = 0; i < length; ++i) {
		printf("%02x ", buffer[i]);
		if((i + 1) % 0x10 == 0) {
			printf("\n");
		}
	}
	printf("\n");
}

int open_file(FILE ** file, const char * filename, const char * mode) {
	if (fopen_s(file, filename, mode) != 0) {
		fprintf(stderr, "Error opening %s.\n", filename);
		return 0;
	}
	return 1;
}

int dump_nand_and_spare_to_files(FILE * nand_file, FILE * spare_file) {
	unsigned char block_buffer[BLOCK_SIZE] = { 0 };
	unsigned char spare_buffer[SPARE_SIZE] = { 0 };

	printf("Reading NAND and spare blocks from the console...\n");
	printf("Blocks read: %.4d (%.2f%%).", 0, 0.0);
	int blk_no;
	for (blk_no = 0; blk_no < NUM_BLOCKS; ++blk_no) {
		if (read_block_spare(block_buffer, spare_buffer, blk_no)) {
			fwrite(block_buffer, sizeof(block_buffer[0]), BLOCK_SIZE, nand_file);
			fwrite(spare_buffer, sizeof(spare_buffer[0]), SPARE_SIZE, spare_file);
			fflush(nand_file);
			fflush(spare_file);
			printf("\rBlocks read: %.4d (%.2f%%).", blk_no + 1, ((blk_no + 1) / 4096.0) * 100.0);
			fflush(stdout);
		}
		else {
			fprintf(stderr, "Error reading block while dumping NAND from the console.\n");
			return 0;
		}
	}

	return 1;
}

/*
	COMMANDS
*/

/*
	When a command is sent to the console, it responds with an 8-byte reply
	in which reply[3] is 0xFF - (command in hex) upon success, otherwise
	there was an error.
*/
static int command_error(uint32_t command, unsigned char * buffer) {
	return buffer[3] != (0xFF - command);
}

/*
	read_block
	One command (READ_BLOCK_ONLY) reads a block only. The other
	(READ_BLOCK_AND_SPARE) reads both the block and the spare area
	of the block's last page.
*/
int read_block_only(unsigned char * block_buffer, uint32_t block_number) {
	if (!request_block_read(READ_BLOCK_ONLY, block_number)) {
		return 0;
	}

	if (!get_block(block_buffer)) {
		fprintf(stderr, "Reading block %04x failed.\n", block_number);
		return 0;
	}

	return 1;
}

int read_block_spare(unsigned char * block_buffer, unsigned char * spare_buffer, uint32_t block_number) {
	if (!request_block_read(READ_BLOCK_AND_SPARE, block_number)) {
		return 0;
	}

	if (!get_block(block_buffer)) {
		fprintf(stderr, "Reading block %04x failed.\n", block_number);
		return 0;
	}

	if (!get_spare(spare_buffer)) {
		fprintf(stderr, "Reading block %04x spare failed.\n", block_number);
		return 0;
	}

	return 1;
}


static int request_block_read(uint32_t command, uint32_t block_number) {	
	if (!ique_send_command(command, block_number)) {
		fprintf(stderr, "Command to read block %04x was sent, but it was not received by the console.\n", block_number);
		return 0;
	}
	
	unsigned char reply_buffer[8] = { 0 };
	if (!ique_receive_reply(reply_buffer, 8)) {
		fprintf(stderr, "Console response to command not received.\nAssuming the worst and aborting block read.\n");
		return 0;
	}
	
	if (command_error(command, reply_buffer)) {
		fprintf(stderr, "The console returned an error when requesting to read block %04x.\n", block_number);
		return 0;
	}
	
	return 1;
}

static int get_block(unsigned char * block_buffer) {
	unsigned char chunk_buffer[BLOCK_CHUNK_SIZE] = { 0 };
	int i;
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

	if (command_error(GET_BBID, reply_buffer)) {
		fprintf(stderr, "The console returned an error when requesting the console's ID.\n");
		return 0;
	}

	// last 4 bytes of the 8-byte reply are the BBID
	*bbid_out = ntohl(*(uint32_t *)&reply_buffer[4]); 
	return 1;
}


