/*
    menu_func.h

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
#ifndef AULON_MENU_FUNC_H
#define AULON_MENU_FUNC_H

// Positions in NAND
enum {
    NAND_START = 0x00, // Start of NAND (obviously)
    FILE_START = 0x40  // After the SKSA area, where the files/filesystem begin
};

int Init(void);
int GetBBID(void);
int SetLED(char * line);
int SignHash(char * line);
int SetTime(void);
int ListFileBlocks(char * line);
int ListFiles(void);
int DumpCurrentFS(void);
int DumpNand(void);
int ReadSingleBlock(char * line);
int WriteNand(int block_start);
int WriteSingleBlock(char * line);
int ReadFile(char * line);
int WriteFile(char * line);
int DeleteFile(char * line);
int PrintStats(void);
int Close(void);


#endif















