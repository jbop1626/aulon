/*
    menu.c
    the aulon main menu and display

    Copyright (c) 2018,2020,2021 Jbop (https://github.com/jbop1626)
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

#include "defs.h"
#include "io.h"
#include "menu_func.h"
#include "menu.h"

#define INPUT_BUFFER_LENGTH 64 // #define because this is used as the declared length of an array
static const char * const version = AULON_VERSION;
static const char * const writing = AULON_WRITING_ENABLED ? " (writing)" : "";
static const char * const logging = AULON_LOGGING_ENABLED ? " (logging)" : "";


void menu_loop(FILE * instream) {
    char * prompt = (instream == stdin ? "> " : "\n");
    printf("aulon v%s%s%s\n", version, writing, logging);
    printf("%s", prompt);
    
    char line[INPUT_BUFFER_LENGTH] = { 0 };
    while (get_input(line, INPUT_BUFFER_LENGTH, instream)) {
        execute_command(line);
        printf("%s", prompt);
    }
}

static void display_help(void) {
    printf("\nCommands:\n\n");
    printf("    B             - Initialize USB connection to the console\n");
    printf("    I             - Request the console's unique BBID\n");
    printf("    H value       - Flash LED\n");
    printf("    S hash_file   - Sign the SHA1 hash in [hash_file] using ECDSA\n");
    printf("    J             - Set console clock to PC's current time\n");
    printf("    L             - List all files currently on the console\n");
    printf("    F             - Dump the current filesystem block to 'current_fs.bin'\n");
    printf("    1             - Dump the console's NAND to 'nand.bin' and 'spare.bin'\n");
    printf("    X blk_num     - Read one block and its spare data from the console to files\n");
#if defined(AULON_WRITING_ENABLED) && (AULON_WRITING_ENABLED == 1)
    printf("    2             - Write partial NAND to the console from files (No SKSA)\n");
    printf("    W             - Write full NAND to the console from files (UNSAFE)\n");
    printf("    Y blk_num     - Write one block to the console from 'block_[blk_num].bin'\n");
#endif
    printf("    3 file        - Read [file] from the console\n");
#if defined(AULON_WRITING_ENABLED) && (AULON_WRITING_ENABLED == 1)
//  printf("    4 file        - Write [file] to the console\n");
//  printf("    R file        - Delete [file] from the console\n");
#endif
    printf("    C             - Print statistics about the console's NAND\n");
    printf("    Q             - Close USB connection to the console\n");
    printf("\n");
    printf("    h             - Print this help (but of course you already know that)\n");
    printf("    ?             - Print copyright and licensing information\n");
    printf("    q             - Quit aulon\n");
    printf("\n");
}

static void display_info(void) {
    printf("\naulon v%s%s%s\n", version, writing, logging);
    printf("Copyright (c) 2018,2019,2020 Jbop (https://github.com/jbop1626)\n");
    printf("aulon is licensed under the GPL v3 (or any later version).\n\n");
    printf("Portions Copyright (c) 2012-2018 Mike Ryan\nOriginally released under the MIT license\n\n");
    printf("libusb is licensed under the LGPL v2.1 (or any later version)\n");
    printf("Copyright (c) 2001 Johannes Erdfelt <johannes@erdfelt.com>\n");
    printf("Copyright (c) 2007 - 2009 Daniel Drake <dsd@gentoo.org>\n");
    printf("Copyright (c) 2010 - 2012 Peter Stuge <peter@stuge.se>\n");
    printf("Copyright (c) 2008 - 2016 Nathan Hjelm <hjelmn@users.sourceforge.net>\n");
    printf("Copyright (c) 2009 - 2013 Pete Batard <pete@akeo.ie>\n");
    printf("Copyright (c) 2009 - 2013 Ludovic Rousseau <ludovic.rousseau@gmail.com>\n");
    printf("Copyright (c) 2010 - 2012 Michael Plante <michael.plante@gmail.com>\n");
    printf("Copyright (c) 2011 - 2013 Hans de Goede <hdegoede@redhat.com>\n");
    printf("Copyright (c) 2012 - 2013 Martin Pieuchot <mpi@openbsd.org>\n");
    printf("Copyright (c) 2012 - 2013 Toby Gray <toby.gray@realvnc.com>\n");
    printf("Copyright (c) 2013 - 2018 Chris Dickens <christopher.a.dickens@gmail.com>\n\n");
    printf("See the included file LIBUSB_AUTHORS.txt for more.\n\n");
}

int execute_command(char * input_line) {
    unsigned char command = input_line[0];
    
    switch (command) {
#if defined(AULON_WRITING_ENABLED) && (AULON_WRITING_ENABLED == 1)
    case 'W':   printf("WriteNand (full) returns %d\n", WriteNand(NAND_START));                 break;
    case '2':   printf("WriteNand (partial) returns %d\n", WriteNand(FILE_START));              break;
    case 'Y':   printf("WriteSingleBlock returns %d\n", WriteSingleBlock(input_line));          break;
//  case '4':   printf("WriteFile returns %u\n", WriteFile(input_line));                        break;
//  case 'R':   printf("DeleteFile returns %u\n", DeleteFile(input_line));                      break;
#endif
    case 'B':   printf("Init returns %u\n", Init());                                            break;
    case 'I':   printf("GetBBID returns %u\n", GetBBID());                                      break;
    case 'H':   printf("SetLED returns %u\n", SetLED(input_line));                              break;
    case 'S':   printf("SignHash returns %u\n", SignHash(input_line));                          break;
    case 'J':   printf("SetTime returns %u\n", SetTime());                                      break;
    case 'K':   printf("ListFileBlocks returns %u\n", ListFileBlocks(input_line));              break;
    case 'L':   printf("ListFiles returns %u\n", ListFiles());                                  break;
    case 'F':   printf("DumpCurrentFS returns %u\n", DumpCurrentFS());                          break;
    case '1':   printf("DumpNand returns %u\n", DumpNand());                                    break;
    case 'X':   printf("ReadSingleBlock returns %d\n", ReadSingleBlock(input_line));            break;
    case '3':   printf("ReadFile returns %u\n", ReadFile(input_line));                          break;
    case 'C':   printf("PrintStats returns %u\n", PrintStats());                                break;
    case 'Q':   printf("Close returns %u\n", Close());                                          break;
    case 'h':   display_help();                                                                 break;
    case '?':   display_info();                                                                 break;
    case 'q':   exit(EXIT_SUCCESS);                                                             break;
    case '\n':  break;
    case '\0':  break;
    default:    fprintf(stderr, "Invalid command. Type 'h' for a list of valid commands.\n");   return 0;
    }
    return 1;
}

