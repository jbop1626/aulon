/*
    main.c

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

#include "defs.h"
#include "usb_log.h"
#include "io.h"
#include "menu.h"


static FILE * input_file = NULL;


static void close_input_file(void) {
    fclose(input_file);
}

static void open_input_file(char * input_file_path) {
    if (!open_file(&input_file, input_file_path, "r")) {
        fprintf(stderr, "Could not open command input file.\n");
        return;
    }
    atexit(close_input_file);
}

static void parse_args(int argc, char * argv[]) {
    int i;
    for (i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], "-f") == 0) {
            open_input_file(argv[i + 1]);
        }
#if defined(AULON_LOGGING_ENABLED) && (AULON_LOGGING_ENABLED == 1)
        else if (strcmp(argv[i], "-l") == 0) {
            usb_log_set_path(argv[i + 1]);
        }
#endif
    }
}


int main(int argc, char * argv[]) {
    parse_args(argc, argv);

    FILE * input_source;
    if (input_file) {
        input_source = input_file;
    }
    else {
        input_source = stdin;
    }

    menu_loop(input_source);

    return 0;
}




