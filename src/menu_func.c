/*
    menu_func.c
    options in the main menu of aulon

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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> // for tolower
#include <time.h>

#include "usb.h"
#include "player_comms.h"
#include "commands.h"
#include "io.h"
#include "fs.h"
#include "menu_func.h"


static int dump_nand_and_spare_to_files(FILE * nand_file, FILE * spare_file);

static int get_unsafe_write_confirmation(void);
static int open_and_check_files(FILE * nand_file, FILE * spare_file);
static int write_nand_and_spare_to_player(FILE * nand_file, FILE * spare_file, int block_start);
static int reading_files_failed(FILE * nand_file,  unsigned char * block_buffer,
                                FILE * spare_file, unsigned char * spare_buffer);

static int save_single_block(unsigned char * block, unsigned char * spare, uint32_t block_num);
static int send_single_block(unsigned char * block, uint32_t block_num);

static int read_hash_from_file(unsigned char * hash, char * hash_filename);
static void print_hash_and_sig(unsigned char * hash, unsigned char * sig);

static int prepare_time_data(uint32_t * first_half, unsigned char * second_half);


int Init(void) {
    if (usb_handle_exists()) {
        fprintf(stderr, "A device is already connected.\nCall Close (Q) to disconnect, and try again.\n\n");
        return 0;
    }
    
    int success = 1;
    if (!usb_init_connection()) {
        success = 0;
    }
    else if (!set_seqno(0x0001)) {
        success = 0;
    }
    else if (!get_num_blocks()) {
        success = 0;
    }
    else if (!get_current_fs()) {
        success = 0;
    }
    else if (!init_fs()) {
        success = 0;
    }
    else if (!delete_file_and_update("temp.tmp")) {
        success = 0;
    }

    if (success) {
        printf("Connection to the device was initialized successfully.\n");      
    }
    else {
        usb_close_connection();
        fprintf(stderr, "Failed to establish a USB connection to the device.\n");
    }

    return success;
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



int SetLED(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    
    unsigned long input = strtoul(line + 2, NULL, 0);
    if (input == 0 || input > UINT32_MAX) {
        fprintf(stderr, "The given value is invalid!\n");
        return 0;
    }

    return set_led((uint32_t) input);
}



int SignHash(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }

    unsigned char hash[SHA1_HASH_LENGTH] = { 0 };
    unsigned char sig[ECC_SIG_LENGTH] = { 0 };
    char * hash_filename = line + 2;
    
    if (!read_hash_from_file(hash, hash_filename)) {
        fprintf(stderr, "Could not read hash from file.\n");
        return 0;
    }
    
    if (!sign_hash(hash, sig)) {
        fprintf(stderr, "Signing hash failed.\n");
        return 0;
    }

    print_hash_and_sig(hash, sig);  
    return 1;
}

static int read_hash_from_file(unsigned char * hash, char * hash_filename) {
    FILE * hash_file = NULL;
    if (!open_file(&hash_file, hash_filename, "rb")) {
        return 0;
    }
    if (get_file_size(hash_file) < SHA1_HASH_LENGTH) {
        fclose(hash_file);
        return 0;
    }
    if (fread(hash, sizeof(hash[0]), SHA1_HASH_LENGTH, hash_file) != SHA1_HASH_LENGTH) {
        fclose(hash_file);
        return 0;
    }
    fclose(hash_file);
    return 1;
}

static void print_hash_and_sig(unsigned char * hash, unsigned char * sig) {
    printf("SHA1 Hash:\n");
    print_buffer(hash, SHA1_HASH_LENGTH, stdout);
    printf("ECC Signature:\n");
    print_buffer(sig, ECC_SIG_LENGTH, stdout);
}



int SetTime(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. No connection is open.\n");
        return 0;
    }
    
    uint32_t time_data_first_half = 0;
    unsigned char time_data_second_half[4] = { 0 };
    if (!prepare_time_data(&time_data_first_half, time_data_second_half)) {
        return 0;
    }
    
    return set_time(time_data_first_half, time_data_second_half);
}

static int prepare_time_data(uint32_t * first_half, unsigned char * second_half) {
    time_t current_time = time(NULL);
    struct tm * t = gmtime(&current_time);
    
    if (t) {
        unsigned char time_data[8] = { 0 };
        time_data[0] = t->tm_year % 100;
        time_data[1] = t->tm_mon + 1;
        time_data[2] = t->tm_mday;
        time_data[3] = t->tm_wday;
        time_data[4] = 0;
        time_data[5] = t->tm_hour;
        time_data[6] = t->tm_min;
        time_data[7] = t->tm_sec;
        
        *first_half = uchars_to_uint32(&time_data[0]);
        memcpy(second_half, &time_data[4], 4);
        return 1;
    }
    else {
        fprintf(stderr, "Conversion to calendar time unsuccessful!\n");
        return 0;
    }
}



int ListFileBlocks(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }

    return list_file_blocks(line + 2);
}



int ListFiles(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }

    list_files();
    return 1;
}



int DumpCurrentFS(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }

    return dump_current_fs();
}



int DumpNand(void) {
    if(!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    
    FILE * nand_file = NULL;
    FILE * spare_file = NULL;
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

static int dump_nand_and_spare_to_files(FILE * nand_file, FILE * spare_file) {
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



int ReadSingleBlock(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    
    unsigned long block_num = strtoul(line + 2, NULL, 0);
    if (block_num >= NUM_BLOCKS) {
        fprintf(stderr, "The given block number is invalid!\n");
        return 0;
    }
    
    int success = 0;
    unsigned char * block = calloc(BLOCK_SIZE, sizeof(unsigned char));
    unsigned char * spare = calloc(SPARE_SIZE, sizeof(unsigned char));
    if (block != NULL && spare != NULL) {
        success = save_single_block(block, spare, (uint32_t)block_num);
    }
    
    free(block);
    free(spare);
    return success;
}

static int save_single_block(unsigned char * block, unsigned char * spare, uint32_t block_num) {
    if (!read_block_spare(block, spare, block_num)) {
        fprintf(stderr, "Could not read single block!\n");
        return 0;        
    }
    else {
        char num[5] = { 0 };
        sprintf(num, "%04X", block_num & 0xFFFF);
        char block_fn[11] = "block_";
        char spare_fn[11] = "spare_";
        strcat(block_fn, num);
        strcat(spare_fn, num);
        
        FILE * block_file = NULL;
        if (!open_file(&block_file, block_fn, "wb")) {
            return 0;
        }
        FILE * spare_file = NULL;
        if (!open_file(&spare_file, spare_fn, "wb")) {
            fclose(block_file);
            return 0;
        }
        
        fwrite(block, sizeof(block[0]), BLOCK_SIZE, block_file);
        fwrite(spare, sizeof(spare[0]), SPARE_SIZE, spare_file);
        fclose(block_file);
        fclose(spare_file);
        return 1;
    }
}



int WriteNand(int block_start) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }

    // If doing an unsafe write, make sure the user definitely wants it.
    if (block_start == NAND_START) {
        if (!get_unsafe_write_confirmation()) {
            printf("Write aborted.\n");
            return 1;
        }
    }

    int success = 1;
    FILE * nand_file = NULL;
    FILE * spare_file = NULL;
    
    if (!open_and_check_files(&nand_file, &spare_file)) {
        success = 0;
    }
    else if (!write_nand_and_spare_to_player(&nand_file, &spare_file, block_start)) {
        success = 0;
    }

    if (fclose(nand_file) || fclose(spare_file)) {
        fprintf(stderr, "Error closing file!\nThe actual write of the NAND file to the console likely succeeded, however.\n");
        success = 0;
    }
    
    if (success) {
        printf("\nNAND write complete!\n");
    } else {
        fprintf(stderr, "\nNAND write failed.\n");
    }
    
    return success;
}

static int get_unsafe_write_confirmation(void) {
    printf("This operation overwrites the area needed to boot your console.\n");
    printf("If you would like a safer NAND write, use the partial write command (2).\n");
    printf("Are you sure you want to write a FULL NAND to the player? (y/n): ");
    char line[10] = { 0 };
    get_input(line, 10, stdin);
    return (tolower(line[0]) == 'y');
}

static int open_and_check_files(FILE ** nand_file, FILE ** spare_file) {
    if (!open_file(spare_file, "spare.bin", "rb") || 
        !open_file(nand_file, "nand.bin", "rb")) {
        return 0;
    }

    if (!file_size_check(*nand_file, BLOCK_SIZE * NUM_BLOCKS)) {
        fprintf(stderr, "nand.bin is not the correct size!\n");
        return 0;
    }

    if (!file_size_check(*spare_file, SPARE_SIZE * NUM_BLOCKS)) {
        fprintf(stderr, "spare.bin is not the correct size!\n");
        return 0;
    }

    return 1;
}

static int write_nand_and_spare_to_player(FILE ** nand_file, FILE ** spare_file, int block_start) {
    unsigned char block_buffer[BLOCK_SIZE] = { 0 };
    unsigned char spare_buffer[SPARE_SIZE] = { 0 };
    double limit = NUM_BLOCKS - block_start;
    int blocks_written = 0;

    if (fseek(*nand_file,  block_start * BLOCK_SIZE, SEEK_SET) != 0 || 
        fseek(*spare_file, block_start * SPARE_SIZE, SEEK_SET) != 0 ) {
        fprintf(stderr, "Error preparing file read!\n");
        return 0;
    }

    printf("Writing NAND and spare blocks to the console...\n");
    printf("Blocks written: %.4d (%.2f%%).", 0, 0.0);
    int blk_no;
    for (blk_no = block_start; blk_no < NUM_BLOCKS; ++blk_no) {
        if (reading_files_failed(*nand_file, block_buffer, *spare_file, spare_buffer)) {
            fprintf(stderr, "Could not read data from NAND or spare files. Aborting NAND write.\n");
            return 0;
        }
        if (write_block_spare(block_buffer, spare_buffer, blk_no)) {
            blocks_written = (blk_no + 1) - block_start;
            printf("\rBlocks written: %.4d (%.2f%%).", blocks_written, (blocks_written / limit) * 100.0);
            fflush(stdout);
        }
        else {
            fprintf(stderr, "Error writing block while writing NAND to the console.\n");
            return 0;
        }
    }

    return 1;
}

static int reading_files_failed(FILE * nand_file, unsigned char * block_buffer, FILE * spare_file, unsigned char * spare_buffer) {
    int read_block_fail = (fread(block_buffer, sizeof(block_buffer[0]), BLOCK_SIZE, nand_file) != BLOCK_SIZE);
    int read_spare_fail = (fread(spare_buffer, sizeof(spare_buffer[0]), SPARE_SIZE, spare_file) != SPARE_SIZE);
    return (read_block_fail) || (read_spare_fail);
}



int WriteSingleBlock(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    
    unsigned long block_num = strtoul(line + 2, NULL, 0);
    if (block_num >= NUM_BLOCKS) {
        fprintf(stderr, "The given block number is invalid!\n");
        return 0;
    }
    
    int success = 0;
    unsigned char * block = calloc(BLOCK_SIZE, sizeof(unsigned char));
    if (block != NULL) {
        success = send_single_block(block, (uint32_t)block_num);
    }
    
    free(block);
    return success;
}

static int get_single_block_write_confirmation(const char * num) {
    printf("Are you sure you wish to overwrite block 0x%s? (y/n)", num);
    char line[10] = { 0 };
    get_input(line, 10, stdin);
    return (tolower(line[0]) == 'y');
}

static int send_single_block(unsigned char * block, uint32_t block_num) {
    char num[5] = { 0 };
    sprintf(num, "%04X", block_num & 0xFFFF);
    if (!get_single_block_write_confirmation(num)) {
        printf("Single block write canceled.\n");
        return 1;
    }
    
    char block_fn[11] = "block_";
    strcat(block_fn, num);
    
    FILE * block_file = NULL;
    if (!open_file(&block_file, block_fn, "rb")) {
        return 0;
    }
    
    if (fread(block, sizeof(block[0]), BLOCK_SIZE, block_file) != BLOCK_SIZE) {
        printf("Error reading block file!\n");
        return 0;
    }
    
    unsigned char spare[SPARE_SIZE] = { 0 };
    memset(spare, 0xFF, SPARE_SIZE * sizeof(spare[0]));
    if (!write_block_spare(block, spare, block_num)) {
        fprintf(stderr, "Could not write single block!\n");
        return 0;
    }
    printf("Single block successfully written to the console!\n");
    return 1;
}



int ReadFile(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    return read_file(line + 2);
}



int WriteFile(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    return write_file(line + 2);
}



int DeleteFile(char * line) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    if (strlen(line) < 3) {
        return 0;
    }
    return delete_file_and_update(line + 2);
}



int PrintStats(void) {
    if (!usb_handle_exists()) {
        fprintf(stderr, "Device handle does not exist. Did you call Init (B)?\n");
        return 0;
    }
    
    print_stats();
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
