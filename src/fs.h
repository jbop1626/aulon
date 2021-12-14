/*
    fs.h

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
#ifndef AULON_FS_H
#define AULON_FS_H

#define FILE_ENTRIES_START 0x2000
#define FILE_ENTRY_SIZE    20
#define NUM_FILE_ENTRIES   409

int get_current_fs(void);
int dump_current_fs(void);
int read_file(const char * filename);
int write_file(const char * filename);
int list_file_blocks(const char * filename);
void list_files(void);
void print_stats(void);
int delete_file_and_update(const char * filename);

#endif
