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

int ique_send_chunked_data(unsigned char * data, size_t data_length);
int ique_send_piecemeal_data(unsigned char * data, size_t data_length);
int ique_send_command(uint32_t command, uint32_t argument);
int ique_send_ack(void);

int ique_receive_reply(unsigned char * buffer, size_t recv_length);

void ique_wait_for_ready(void);

#endif
