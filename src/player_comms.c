/*
	player_comms.c
	the basic communication and data transfer protocol used by the iQue Player.

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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
	#include <winsock2.h>
#else
	#include <arpa/inet.h>
#endif

#include "usb.h"
#include "player_comms.h"

#define PACKET_SIZE 0x40 // #define because this is used as the declared length of an array
static const unsigned char SEND_CHUNK_SIGNAL = 0x63;

static int ique_is_ready(void);
static int ique_receive_data_length(void);
static int ique_receive_data(unsigned char * buffer, size_t data_length);
static int parse_received_data(unsigned char * in_buffer,  size_t total_data_received,
                               unsigned char * out_buffer, size_t expected_data_length);

/*
	SEND data
	The host computer can send data to the iQue Player using 2 methods:

	chunked data:
		The host sends large chunks of data all at once. Each chunk starts with
		0x63 and a byte representing the length of the data. Thus to send 254 bytes
		to the player, the chunk would be { 63 FE [254 bytes of actual data] }.

		0xFE is the largest size for a single chunk that has been observed (though
		clearly the size couldn't be larger than 0xFF anyway, since it must fit
		into a single byte).

	piecemeal data:
		The host sends data gradually, with 1-byte tags marking off 3-, 2-, or
		1-byte sections. The tags are in the form 0x40 + num_bytes.

		Commands (among other things) are sent in this format.
*/

int ique_send_chunked_data(unsigned char * data, size_t data_length) {
    unsigned char chunk_buffer[0x100] = { SEND_CHUNK_SIGNAL };
    unsigned int remaining_data = data_length;
    unsigned int offset = 0;
    int transferred;

    while (remaining_data) {
        unsigned char chunk_length = (remaining_data >= 0xFE) ? 0xFE : (unsigned char)remaining_data;
        chunk_buffer[1] = chunk_length;
        memcpy(chunk_buffer + 2, data + offset, chunk_length);
        if (!usb_bulk_transfer_send(chunk_buffer, chunk_length + 2, &transferred, 0)) {
            fprintf(stderr, "Error when sending chunk of data to the player.\n");
            return 0;
        }
        remaining_data -= chunk_length;
        offset += chunk_length;
    }

    return 1;
}


int ique_send_piecemeal_data(unsigned char * data, size_t data_length) {
	// data_length + space for tags every 3 bytes
	size_t send_data_length = data_length + (data_length / 3) + (data_length % 3 != 0); 
	unsigned char * send_data = calloc(send_data_length, sizeof(unsigned char));
	if (send_data == NULL) {
		fprintf(stderr, "calloc failed when sending data.\n");
		return 0;
	}

	size_t remaining_data = data_length;
	size_t send_offset = 0;
	size_t in_offset = 0;
	while (remaining_data) {
		unsigned char bytes_processed = (remaining_data >= 3) ? 3 : (unsigned char)remaining_data;
		send_data[send_offset] = 0x40 + bytes_processed;
		memcpy(send_data + send_offset + 1, data + in_offset, bytes_processed);

		send_offset += bytes_processed + 1;
		in_offset += bytes_processed;
		remaining_data -= bytes_processed;
	}	

	int transferred;
	if (!usb_bulk_transfer_send(send_data, send_data_length, &transferred, 0)) {
		fprintf(stderr, "Error when sending piecemeal data to the player.\n");
		free(send_data);
		return 0;
	}

	free(send_data);
	return 1;
}


int ique_send_command(uint32_t command, uint32_t argument) {
	uint32_t message[2] = { htonl(command), htonl(argument) };
	return ique_send_piecemeal_data((unsigned char *)message, 8);
}


int ique_send_ack(void) {
	int transferred;
	unsigned char ack = 0x44;

	return usb_bulk_transfer_send(&ack, 1, &transferred, 0);
}

/*
	RECEIVE data

	Before sending out data, the player first sends a 4-byte signal containing
	the length of the data it is about to send. This signal starts with 0x1B,
	and the remaining 3 bytes give the size of the data (in bytes).

	The actual data is sent from the player in a format analogous to the
	piecemeal format described above. The player sends data gradually to the
	host, with 1-byte tags marking off 3-, 2-, or 1-byte sections. The tags
	are in the form	0x1C + num_bytes.
*/

int ique_receive_reply(unsigned char * buffer, size_t recv_length) {
	int data_length = ique_receive_data_length();
	if (data_length == -1) {
		return 0;
	}
	if ((unsigned int)data_length > recv_length) {
		fprintf(stderr, "Amount of data in reply exceeds the size of the allocated buffer:\n%d vs %d.\n\n", data_length, recv_length);
		return 0;
	}
	return ique_receive_data(buffer, (size_t)data_length);
}


static int ique_receive_data_length(void) {
	unsigned char length_buffer[4] = { 0 };
	int transferred;

	if (!usb_bulk_transfer_receive(length_buffer, 4, &transferred, 0)) {
		return -1;
	}
	if (length_buffer[0] == 0x15) {
		// Received a ready signal instead -- try *one* more time.
		if (!usb_bulk_transfer_receive(length_buffer, 4, &transferred, 0)) {
			return -1;
		}
	}
	if (length_buffer[0] != 0x1B) {
		fprintf(stderr, "Unknown transfer unit type encountered when receiving reply length: %x\n", length_buffer[0]);
		return -1;
	}

	length_buffer[0] = 0;
	uint32_t data_length = ntohl(*(uint32_t *)length_buffer);
	return data_length;
}


static int ique_receive_data(unsigned char * buffer, size_t data_length) {
	// data_length + space for tags every 3 bytes (+ extra in case the player is inefficient)
	size_t recv_buffer_length = data_length + ((data_length / 3) + 4); 
	unsigned char * recv_buffer = calloc(recv_buffer_length, sizeof(unsigned char));
	if (recv_buffer == NULL) {
		fprintf(stderr, "calloc failed when receiving data.\n");
		return 0;
	}

	unsigned int total_data_received = 0;
	unsigned char packet[PACKET_SIZE] = { 0 };
	int transferred = PACKET_SIZE;

	// Read one packet at a time; end after we receive a packet that is not full.
	// This models the way USB receives packets normally and ensures all data sent
	// from the console is read -- even if its more or less than expected.
	while (transferred == PACKET_SIZE) {
		if (usb_bulk_transfer_receive(packet, PACKET_SIZE, &transferred, 0)) {
			memcpy(recv_buffer + total_data_received, packet, transferred);
			total_data_received += transferred;
		}
		else {
			free(recv_buffer);
			fprintf(stderr, "Error receiving data.\n");
			return 0;
		}
	}

	ique_send_ack();
	return parse_received_data(recv_buffer, total_data_received, buffer, data_length);
}


static int parse_received_data(unsigned char * in_buffer,  size_t total_data_received,
                               unsigned char * out_buffer, size_t expected_data_length) {
	unsigned int in_offset = 0;
	unsigned int copied_data = 0;

	while (copied_data < expected_data_length && in_offset < total_data_received) {
		unsigned int tu = in_buffer[in_offset];
		if (tu == 0x1F || tu == 0x1E || tu == 0x1D) {
			unsigned int tu_length = tu - 0x1C;
			memcpy(out_buffer + copied_data, in_buffer + in_offset + 1, tu_length);
			copied_data += tu_length;
		}
		else {
			fprintf(stderr, "Unknown transfer unit type encountered when parsing received data: %x\n", tu);
			free(in_buffer);
			return 0;
		}
		in_offset += 4;
	}

	free(in_buffer);
	if (copied_data != expected_data_length) {
		fprintf(stderr, "Received data is not equal to the expected transfer size.\n");
		return 0;
	}
	return 1;
}


/*
	Wait for the ready signal (15 00 00 00)
*/
void ique_wait_for_ready(void) {
	while (!ique_is_ready());
}


static int ique_is_ready(void) {
	const unsigned char ready_signal[4] = { 0x15, 0, 0, 0 };
	unsigned char buffer[4] = { 0 };

	int transferred;
	if (!usb_bulk_transfer_receive(buffer, 4, &transferred, 0)) {
		return 0;
	}
	if (transferred < 4) {
		return 0;
	}

	return memcmp(buffer, ready_signal, 4) == 0;
}

