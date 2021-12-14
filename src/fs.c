/*
    fs.c
    Routines for working with the iQue Player file system over USB

    Copyright (c) 2020 Jbop (https://github.com/jbop1626)
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
#include <string.h>

#include "fs.h"
#include "io.h"
#include "commands.h"

static unsigned char current_fs[BLOCK_SIZE];
static unsigned char current_sp[SPARE_SIZE];
static uint32_t current_index = 0;

/*
    Simple utility functions
*/
static void construct_filename(char * filename, size_t index) {
    strncat(filename, (char *)&current_fs[index], 8);
    filename[strlen(filename)] = '.';
    strncat(filename, (char *)&current_fs[index + 8], 3);
}

static int set_filename(size_t index, const char * new_fn) {
    size_t full_len = strlen(new_fn);
    size_t fn_len   = strcspn(new_fn, ".");
    size_t ext_len  = full_len - fn_len - 1;

    if (full_len > 12 || fn_len > 8 || ext_len > 3) {
        fprintf(stderr, "Error setting filename: Filename invalid!\n");
        return 0;
    }
    
    memset(&current_fs[index], 0, 11);
    memcpy(&current_fs[index], new_fn, fn_len);
    memcpy(&current_fs[index + 8], (new_fn + fn_len + 1), ext_len);
    return 1;
}

static int entry_valid(size_t index) {
    if (current_fs[index] == 0) {
        // Filename (and probably the entire entry) is NULL
        return 0;
    }
    if (current_fs[index + 0xB] == 0) {
        // File marked invalid
        return 0;
    }
    else if (uchars_to_int16(&current_fs[index + 0xC]) == -1) {
        // Start block for the file is -1
        return 0;
    }
    return 1;
}

static size_t find_file(const char * filename) {
    size_t result = 0;
    for (size_t i = 0; i < NUM_FILE_ENTRIES; ++i) {
        size_t index = FILE_ENTRIES_START + (i * FILE_ENTRY_SIZE);
        if (entry_valid(index)) {
            char test_fn[13] = { 0 };
            construct_filename(test_fn, index);
            if (strcmp(filename, test_fn) == 0) {
                result = index;
                break;
            }
        }
        
    }
    return result;
}

static int rename_file(const char * old_fn, const char * new_fn) {
    size_t index = find_file(old_fn);
    if (index == 0) {
        fprintf(stderr, "Error renaming file: File to rename does not exist!\n");
        return 0;
    }

    return set_filename(index, new_fn);
}

static uint32_t bytes_to_blocks(uint32_t bytes) {
    return (bytes / BLOCK_SIZE) + (bytes % BLOCK_SIZE != 0);
}

static uint32_t get_file_block_count(const char * filename) {
    size_t index = find_file(filename);
    if (index == 0) {
        fprintf(stderr, "Error calculating block count of file: file not found\n");
        return 0;
    }
    return bytes_to_blocks(uchars_to_uint32(&current_fs[index + 0x10]));
}

static uint32_t get_free_block_count(void) {
    uint32_t result = 0;
    for (int i = 0; i < 0x2000; i+=2) {
        if (uchars_to_int16(&current_fs[i]) == 0)
            result++;
    }
    return result;
}



/*
    Write the current filesystem to a file on the host computer.
    This could be especially useful for trying to update the FS manually
    if automatically updating the FS failed.
*/
int dump_current_fs(void) {
    FILE * file = NULL;
    if (!open_file(&file, "current_fs.bin", "wb")) {
        fprintf(stderr, "Could not dump current filesystem!\n");
        return 0;
    }
    
    fwrite(current_fs, sizeof(current_fs[0]), BLOCK_SIZE, file);
    fclose(file);
    return 1;
}



/*
    Update the console's filesystem by sending the current_fs with all of its changes.
*/
static void increment_seqno(void) {
    uint32_t seqno = uchars_to_uint32(&current_fs[0x3FF8]);
    seqno++;
    current_fs[0x3FF8] = (seqno & 0xFF000000) >> 24;
    current_fs[0x3FF9] = (seqno & 0x00FF0000) >> 16;
    current_fs[0x3FFA] = (seqno & 0x0000FF00) >>  8;
    current_fs[0x3FFB] = (seqno & 0x000000FF);
}

static int update_fs(void) {
    uint32_t next_index = ((current_index - 1) % 16) + 0xFF0;
    
    increment_seqno();    
    
    if (!write_block_spare(current_fs, current_sp, next_index)) {
        fprintf(stderr, "Could not update filesystem! The block to be written was %u.\n", next_index);
        fprintf(stderr, "The filesystem to be written will be dumped to a file named 'current_fs.bin'\n");
        dump_current_fs();
        return 0;
    }
    
    if (!init_fs()) {
        fprintf(stderr, "Filesystem not synchronized! Resetting the console should do it for you.\n");
    }
    current_index = next_index;
    return 1;
}



/*
    Find the current up-to-date filesystem and its block
*/
static uint32_t check_seqno(unsigned char * block, unsigned char * spare,
                            uint32_t block_num, uint32_t current_seqno) {
    if (!read_block_spare(block, spare, block_num)) {
        fprintf(stderr, "Unable to read all FS blocks!\n");
        return 0;
    }
    
    uint32_t seqno = uchars_to_uint32(&block[0x3FF8]);
    if (seqno > current_seqno) {
        memcpy(current_fs, block, BLOCK_SIZE);
        memcpy(current_sp, spare, SPARE_SIZE);
        current_index = block_num - 0xFF0;
        return seqno;
    }
    
    return current_seqno;
}

int get_current_fs(void) {
    uint32_t current_seqno = 0;
    
    unsigned char * block_temp = calloc(BLOCK_SIZE, sizeof(unsigned char));
    unsigned char * spare_temp = calloc(SPARE_SIZE, sizeof(unsigned char));
    if (block_temp == NULL || spare_temp == NULL) {
        fprintf(stderr, "Could not allocate memory for analyzing FS!\n");
    }
    else {
        for (uint32_t i = 0xFFF; i >= 0xFF0; --i) {
            current_seqno = check_seqno(block_temp, spare_temp, i, current_seqno);
        }
    }

    free(block_temp);
    free(spare_temp);
    return (current_seqno != 0);
}



/*
    List the numbers of the blocks that make up the given file.
*/
int list_file_blocks(const char * filename) {
    size_t index = find_file(filename);
    if (index == 0) {
        fprintf(stderr, "The given file is not present on the console.\n");
        return 0;
    }
    
    int16_t next_block = uchars_to_int16(&current_fs[index + 0xC]);
    unsigned count = 0;
    while (next_block >= 0) {
        count++;
        printf("Block %u: 0x%04x\n", count, next_block);
        next_block = uchars_to_int16(&current_fs[next_block * 2]);
    }
    return 1;
}



/*
    Print all files currently on the console with their sizes.
*/
static void print_file_entry(size_t entry_no, unsigned * count) {
    size_t index = FILE_ENTRIES_START + (entry_no * FILE_ENTRY_SIZE);
    if (entry_valid(index)) {
        char filename[13] = { 0 };
        construct_filename(filename, index);
        
        uint32_t file_size = uchars_to_uint32(&current_fs[index + 0x10]);
        unsigned num_blocks = file_size / BLOCK_SIZE;
        const char * s = (num_blocks == 1) ? "" : "s";

        (*count)++;
        printf("%u. %s (%u bytes, %u block%s)\n", *count, filename, file_size, num_blocks, s);
    }
}

void list_files(void) {
    unsigned count = 0;
    for (size_t i = 0; i < NUM_FILE_ENTRIES; ++i) {
        print_file_entry(i, &count);
    }
}



/*
    Delete a file on the console.
*/
static void free_blocks(size_t index) {
    int16_t next_block = uchars_to_int16(&current_fs[index + 0xC]);
    while (next_block >= 0) {
        int16_t curr_block = next_block;
        next_block = uchars_to_int16(&current_fs[curr_block * 2]);
        current_fs[(curr_block * 2)]     = 0;
        current_fs[(curr_block * 2) + 1] = 0;
    }
}

static void delete_file_entry(size_t index) {
    memset(&current_fs[index], 0, 20);
}

static int delete_file(const char * filename) {
    size_t index = find_file(filename);
    if (index == 0) {
        return 0;
    }
    
    free_blocks(index);
    delete_file_entry(index);
    return 1;
}

int delete_file_and_update(const char * filename) {
    if (delete_file(filename)) {
        return update_fs();
    }
    else {
        return 1;
    }
}



/*
    Print the number of currently free, used, and bad blocks, and
    the sequence number of the current filesystem.
*/
void print_stats(void) {
    size_t free_count = 0;
    size_t used_count = 0;
    size_t bad_count = 0;
    
    int16_t temp = 0;
    for (int i = 0; i < 0x2000; i+=2) {
        temp = uchars_to_int16(&current_fs[i]);
        
        if (temp == 0)
            free_count++;
        else if (temp == -2)
            bad_count++;
        else
            used_count++;
    }
    
    uint32_t seqno = uchars_to_uint32(&current_fs[0x3FF8]);
    printf("Free: %zu\nUsed: %zu\nBad: %zu\nSequence Number: %d\n", free_count, used_count, bad_count, seqno);
}



/*
    Read a file from the console to a file on the host computer.
*/
static int read_blocks_to_file(size_t entry_index, FILE * file) {
    int success = 1;
    unsigned char * block_temp = calloc(BLOCK_SIZE, sizeof(unsigned char));
    unsigned char * spare_temp = calloc(SPARE_SIZE, sizeof(unsigned char));
    if (block_temp == NULL || spare_temp == NULL) {
        fprintf(stderr, "Could not allocate memory to read file from console!\n");
        success = 0;
    }
    else {
        int16_t next_block = uchars_to_int16(&current_fs[entry_index + 0xC]);
        while (next_block >= 0) {
            if (!read_block_spare(block_temp, spare_temp, next_block)) {
                fprintf(stderr, "Unable to read block %x while reading file from console!\n", next_block);
                success = 0;
                break;
            }
            
            fwrite(block_temp, sizeof(unsigned char), BLOCK_SIZE, file);
            next_block = uchars_to_int16(&current_fs[next_block * 2]);
        }
    }
    
    free(block_temp);
    free(spare_temp);
    return success;
}

int read_file(const char * filename) {
    if (strlen(filename) > 12) {
        fprintf(stderr, "Filename invalid: Too long for iQue Player FS.\n");
        return 0;
    }
    
    size_t index = find_file(filename);
    if (index == 0) {
        fprintf(stderr, "The given file is not present on the console.\n");
        return 0;
    }
    
    FILE * pc_file = NULL;
    if (!open_file(&pc_file, filename, "wb")) {
        fprintf(stderr, "Could not open a file to retrieve data from the console.\n");
        return 0;
    }
    
    int success = 1;
    if (!read_blocks_to_file(index, pc_file)) {
        fprintf(stderr, "Could not read the console's filesystem!\n");
        success = 0;
    }

    fclose(pc_file);
    return success;
}



/*
    Write a file from a file on the host computer to the console.
*/
static size_t read_data_for_checksum(unsigned char * buffer, FILE * file) {
    if (feof(file)) {
        return 0;
    }
    
    size_t read = fread(buffer, sizeof(unsigned char), BLOCK_SIZE, file);
    if (ferror(file)) {
        fprintf(stderr, "Error reading file!\n");
        return 0;
    }
    
    return read;
}

static uint32_t calculate_file_checksum(FILE * file, size_t size) {
    unsigned char * buffer = calloc(BLOCK_SIZE, sizeof(unsigned char));
    if (file == NULL || buffer == NULL) {
        fprintf(stderr, "Could not calculate file checksum!\n");
        return 0;
    }
    
    uint32_t checksum = 0;
    size_t total_read = 0;
    size_t read_count = 0;
    while (total_read < size) {
        read_count = read_data_for_checksum(buffer, file);
        if (read_count == 0) {
            break;
        }

        for (size_t i = 0; i < read_count; ++i) {
            checksum += buffer[i];
        }
        total_read += read_count;
    }
    
    if (total_read != size) {
        fprintf(stderr, "Error: Complete file contents were not read and factored into checksum.\n");
        checksum = 0;
    }
    
    free(buffer);
    return checksum;
}

static int validate_file_write(const char * filename, uint32_t checksum, uint32_t blocks_required) {
    size_t index = find_file(filename);
    if (index && file_checksum_cmp(filename, checksum, blocks_required * BLOCK_SIZE)) {
        fprintf(stderr, "Exact file to be written already exists on the console!\n");
        return 0;
    }
    
    uint32_t extra = index ? get_file_block_count(filename) : 0;
    if (blocks_required >= (get_free_block_count() + extra)) {
        fprintf(stderr, "Not enough free blocks to write file!\n");
        return 0;
    }
    
    if (index) {
        delete_file(filename);
    }
    return 1;
}

static int write_file_blocks(FILE * file, int16_t * blocks_to_write, uint32_t num_blocks) {
    
    unsigned char * block = calloc(BLOCK_SIZE, sizeof(unsigned char));
    if (block == NULL || file == NULL || blocks_to_write == NULL) {
        fprintf(stderr, "Could not perform file write operation!\n");
        free(block);
        return 0;
    }
    unsigned char spare[20] = { 0 };
    memset(spare, 0xFF, SPARE_SIZE);
    
    int success = 1;
    for (size_t i = 0; i < num_blocks; ++i) {
        if (feof(file)) {
            fprintf(stderr, "Unexpected end-of-file when writing file to console!\n");
            success = 0;
            break;
        }
        
        memset(block, 0, BLOCK_SIZE);
        size_t read_count = fread(block, sizeof(block[0]), BLOCK_SIZE, file);
        if (ferror(file) || read_count == 0) {
            fprintf(stderr, "Error reading from source file during file write operation!\n");
            success = 0;
            break;
        }
        
        if (!write_block_spare(block, spare, blocks_to_write[i])) {
            fprintf(stderr, "Error writing block to console during file write!\n");
            success = 0;
            break;
        }
    }
    
    free(block);
    return success;
}

static size_t find_blank_file_entry(void) {
    size_t result = 0;
    unsigned char blank_entry[20] = { 0 };
    for (size_t i = 0; i < NUM_FILE_ENTRIES; ++i) {
        size_t index = FILE_ENTRIES_START + (i * FILE_ENTRY_SIZE);
        if (memcmp(&current_fs[index], blank_entry, 20) == 0) {
            result = index;
            break;
        }
    }
    return result;
}

static int write_file_entry(const char * filename, int16_t start_block, uint32_t file_size) {
    size_t index = find_blank_file_entry();
    if (index == 0) {
        fprintf(stderr, "No more files can be written to the console.\nAt least one will have to be deleted to create space.\n");
        return 0;
    }
    
    if (set_filename(index, filename)) {
        current_fs[index + 0xB]  = 1;
        current_fs[index + 0xC]  = (start_block & 0xFF00) >> 8;
        current_fs[index + 0xD]  = (start_block & 0x00FF);
        current_fs[index + 0x10] = (file_size & 0xFF000000) >> 24;
        current_fs[index + 0x11] = (file_size & 0x00FF0000) >> 16;
        current_fs[index + 0x12] = (file_size & 0x0000FF00) >>  8;
        current_fs[index + 0x13] = (file_size & 0x000000FF);
        return 1;
    }
    else {
        fprintf(stderr, "Could not create an entry in the filesystem for the given file!\n");
        return 0;
    }
}

static int16_t find_next_free_block(int16_t start_block_num) {
    int16_t result = -1;
    for (int16_t i = (start_block_num * 2); i < 0x2000; i+=2) {
        int16_t temp = uchars_to_int16(&current_fs[i]);
        if (temp == 0) {
            result = i;
            break;
        }
    }
    return (result / 2);
}

static void update_fs_links(int16_t * blocks_to_write, int16_t start_block, uint32_t num_blocks) {
    int16_t current_blk = start_block;
    int16_t next_blk = 0;
    uint32_t blocks_remaining = num_blocks;
    size_t i = 0;
    
    while (blocks_remaining > 1) {
        blocks_to_write[i] = current_blk;
        
        next_blk = find_next_free_block(current_blk + 1);
        current_fs[current_blk * 2]     = (next_blk & 0xFF00) >> 8;
        current_fs[current_blk * 2 + 1] = (next_blk & 0x00FF);
        
        current_blk = next_blk;
        blocks_remaining--;
        i++;
    }
    
    blocks_to_write[i] = current_blk;
    current_fs[current_blk * 2]     = 0xFF;
    current_fs[current_blk * 2 + 1] = 0xFF;
}

static int write_blocks_to_temp_file(FILE * file, uint32_t blocks_required) {
   
    int16_t start_block = find_next_free_block(0x40);
    if (start_block == -1 || !write_file_entry("temp.tmp", start_block, blocks_required * BLOCK_SIZE)) {
        return 0;
    }
    
    int success = 1;
    int16_t * blocks_to_write = calloc(blocks_required, sizeof(int16_t));
    if (blocks_to_write == NULL) {
        success = 0;
    }
    else {
        update_fs_links(blocks_to_write, start_block, blocks_required);
        if (!write_file_blocks(file, blocks_to_write, blocks_required)) {
            fprintf(stderr, "Could not write file data to the console!\n");
            success = 0;
        }
    }

    free(blocks_to_write);
    return success;
}

static int check_and_cleanup_temp_file(const char * filename, uint32_t checksum, uint32_t blocks_required) {
    if (file_checksum_cmp("temp.tmp", checksum, blocks_required * BLOCK_SIZE)) {
        if (!rename_file("temp.tmp", filename)) {
            fprintf(stderr, "Could not rename temp.tmp file!\n");
            return 0;
        }
    }
    else {
        fprintf(stderr, "Checksum of file written to the console is incorrect!\n");
        return 0;
    }
    return 1;
}

int write_file(const char * filename) {
    FILE * pc_file = NULL;
    if (!open_file(&pc_file, filename, "rb")) {
        return 0;
    }
    
    size_t pc_file_size = get_file_size(pc_file);
    uint32_t pc_file_checksum = calculate_file_checksum(pc_file, pc_file_size);
    uint32_t blocks_required = bytes_to_blocks(pc_file_size);
    
    int success = 0;
    if (pc_file_size > UINT32_MAX || blocks_required > 0xFB0) {
        fprintf(stderr, "File is too large to be written to the console!\n");
    }
    else if (pc_file_checksum == 0) {
        fprintf(stderr, "Could not calculate checksum of file to be written!\n");
    }
    else if (!validate_file_write(filename, pc_file_checksum, blocks_required)) {
        fprintf(stderr, "File write operation aborted.\n");
    }
    else if (!write_blocks_to_temp_file(pc_file, blocks_required)) {
        fprintf(stderr, "Error writing file to the console!\n");
    }
    else if (!check_and_cleanup_temp_file(filename, pc_file_checksum, blocks_required)) {
        fprintf(stderr, "Error verifying or cleaning up 'temp.tmp' file after file write!\n");
    }
    else {
        success = 1;
    }
    
    // update FS even if a write errors, because it should be as up-to-date as possible
    update_fs();

    fclose(pc_file);
    return success;
}


