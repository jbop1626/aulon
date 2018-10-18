/*
    menu_func.c
    options in the main menu of aulon

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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "usb.h"
#include "player_comms.h"
#include "operations.h"
#include "menu_func.h"


int Init(void) {
    if(usb_handle_exists()) {
        fprintf(stderr, "A device is already connected.\nCall Close (Q) to disconnect, and try again.\n\n");
        return 0;
    }

    if(!usb_init_connection()) {
        return 0;
    }

    unsigned char reply_buffer[8] = { 0 };
        
    ique_send_command(0x0017, 0x0000);
    ique_receive_reply(reply_buffer, 8);

    printf("Connection to the device was initialized successfully.\n");
    return 1;
}


int Close(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. No connection is open.\n");
        return 0;
    }
    if(!usb_close_connection()) {
        fprintf(stderr, "Could not close USB connection.\n");
        return 0;
    }
    printf("Connection to current device closed.\n");
    return 1;
}


int GetBBID(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }

    uint32_t BBID = 0;
    int r = get_bbid(&BBID);
    printf("BBID returned by the console is %04x.\n", BBID);
    return r;
}


int DumpNand(void) {
    if(!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    
    FILE * nand_file;
    FILE * spare_file;

    if (!open_file(&spare_file, "spare.bin", "wb")) {
        return 0;
    }
    if (!open_file(&nand_file, "nand.bin", "wb")) {
        fclose(spare_file);
        return 0;
    }
    if (!dump_nand_and_spare_to_files(nand_file, spare_file)) {
        fclose(nand_file);
        fclose(spare_file);
        return 0;
    }

    fclose(nand_file);
    fclose(spare_file);
    printf("\nNAND dump complete!\n");
    return 1;
}

