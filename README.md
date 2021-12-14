## aulon
aulon is an open-source, cross-platform tool for communicating with the [iQue Player](https://en.wikipedia.org/wiki/IQue_Player) over USB, intended as a modern replacement for the official ```ique_diag``` program found in the latest iQue@Home version. Unlike ique_diag, which only works on Windows XP, aulon should work on any common PC platform.  

### Installation
On many platforms libusb should already be installed, or can easily be installed using a package manager.  

On Windows you must install a libusb-compatible driver like WinUSB (see the [libusb Windows guide](https://github.com/libusb/libusb/wiki/Windows) for more information). This is most easily done with [Zadig](http://zadig.akeo.ie/). You can find the needed VID and PID [here](https://github.com/jbop1626/aulon/blob/master/src/usb.c#L34).  

If necessary, [download or compile](https://github.com/libusb/libusb/releases) a libusb-1.0 binary, make it available to the program, and then it should be good to go.  

### Building
See [Building.md](https://github.com/jbop1626/aulon/blob/master/build/BUILDING.md)

### Licensing
aulon is licensed under the GPL (v3, or any later version), a copy of which should be in the root directory.  

Modified portions of [ique.c](https://github.com/mikeryan/ique_dumper/blob/master/ique.c) from [ique_dumper](https://github.com/mikeryan/ique_dumper) by Mike Ryan are used in [usb.c](https://github.com/jbop1626/aulon/blob/master/src/usb.c). This code was released under the MIT License as contained in [LICENSES/MIT.txt](https://github.com/jbop1626/aulon/blob/master/LICENSES/MIT.txt)  

aulon currently uses [libusb](https://github.com/libusb/libusb) (v1.0) to access the the console over USB. libusb is released under the LGPL as contained in [LICENSES/LGPL.txt](https://github.com/jbop1626/aulon/blob/master/LICENSES/LGPL.txt)  

### Known Issues
If running aulon through Windows command prompt, if you have the QuickEdit or Insert mode enabled (can be found by right-clicking the title bar and selecting "Properties") and click on the aulon window during a read or write, the operation may stall and fail. To remedy this, you can disable those modes in the menu, make sure not to click in the aulon window during a read/write operation, and/or (if the pause does happen) press enter immediately to unblock the operation and continue. This problem doesn't affect other terminals.   
