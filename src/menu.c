/*
	menu.c
	the aulon main menu and display

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "menu_func.h"
#include "menu.h"

static const char * const aulon_version = "aulon v0.0.2";

void menu_loop(void) {
	printf("%s\n", aulon_version);
	printf("> ");
	char line[30] = { 0 };
	while (get_input(line, 30)) {
		execute_command(line, 30);
		printf("> ");
	}
}

int get_input(char * line_buffer, int buffer_length) {
	char * result = fgets(line_buffer, buffer_length, stdin);
	if (result == NULL) {
		return 0;
	}

	// Discard extra characters in stdin if necessary
	result = (char *)memchr(line_buffer, '\n', 30);
	if (result == NULL) {
		while (getchar() != '\n');
	}

	return 1;
}

void display_help(void) {
	printf("\nCommands:\n\n");
	printf("    B             - Init USB connection to the console\n");
	printf("    I             - Request the console's unique BBID\n");
	printf("    1             - Dump the console's NAND to 'nand.bin' and 'spare.bin'\n");
	printf("    Q             - Close USB connection to the console\n");
	printf("\n");
	printf("    h             - Print this help (but of course you already know that)\n");
	printf("    q             - Quit aulon\n");
	printf("\n");
}

void display_info(void) {
	printf("\n%s\n", aulon_version);
	printf("Copyright (c) 2018 Jbop (https://github.com/jbop1626)\n");
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

int execute_command(char * line, int length) {
	unsigned char command = line[0];
	switch (command) {
		case 'B':
			printf("Init returns %d\n", Init());
			break;
		case 'I':
			printf("GetBBID returns %d\n", GetBBID());
			break;
		case '1':
			printf("DumpNand returns %d\n", DumpNand());
			break; 
		case 'Q':
			printf("Close returns %d\n", Close());
			break;
		case 'h':
			display_help();
			break;
		case 'q':
			exit(EXIT_SUCCESS);
			break;
		case '?':
			display_info();
			break;
		case '\n':
			break;
		default:
			fprintf(stderr, "Invalid command. Type 'h' for a list of valid commands.\n");
			return 0;
	}
	return 1;
}
