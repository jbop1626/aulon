/*
    defs.h
    general definitions across aulon

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
#ifndef AULON_DEFS_H
#define AULON_DEFS_H

#define AULON_VERSION "1.0.0"

// Toggle between 0 (off) and 1 (on). If on, commands for writing both NAND
// and individual files are enabled. This is off by default for safety.
#define AULON_WRITING_ENABLED 0

// Toggle between 0 (off) and 1 (on). If on, aulon takes an optional command
// line argument "-l <path_to_log_file>". Each usb transfer is appended to
// the log file with direction and length information (this is important to
// remember if you are e.g. dumping a NAND -- the entirety of the NAND and
// then some will be written to the log in addition to the regular dump file!).
#define AULON_LOGGING_ENABLED 0

#endif

