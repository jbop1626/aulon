/*
	player_comms.h

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
#ifndef AULON_PLAYER_COMMS_H
#define AULON_PLAYER_COMMS_H

#include <stdint.h>

// USB command numbers to be used with ique_send_command
enum {
	WRITE_BLOCK_ONLY		= 0x06,
	READ_BLOCK_ONLY			= 0x07,
//	UNKNOWN_COMMAND			= 0x0D, // Scan bad blocks; needs more testing
	WRITE_BLOCK_AND_SPARE	= 0x10,
	READ_BLOCK_AND_SPARE	= 0x11,
	INIT_FS					= 0x12,
	GET_NUM_BLOCKS			= 0x15,
	SET_SEQNO				= 0x16,
	GET_SEQNO				= 0x17,
	FILE_CHKSUM				= 0x1C,
	SET_LED					= 0x1D,
	SET_TIME				= 0x1E,
	GET_BBID				= 0x1F,
	SIGN_HASH				= 0x20
};

int ique_send_chunked_data(unsigned char * data, size_t data_length);
int ique_send_piecemeal_data(unsigned char * data, size_t data_length);
int ique_send_command(uint32_t command, uint32_t argument);
int ique_send_ack(void);
int ique_receive_reply(unsigned char * buffer, size_t recv_length);
void ique_wait_for_ready(void);

#endif

