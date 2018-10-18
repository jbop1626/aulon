### Windows
This guide assumes you have  Visual Studio 2017 installed. If not, you can download the Community edition for free [here](https://visualstudio.microsoft.com/downloads/).  

#### Part 1: Compiling libusb  
Compiling libusb isn't strictly necesssary -- you can download pre-built libs from the [libusb website](https://libusb.info/). However, if you have problems with the pre-built libs or want to make your own modifications, you should compile from source following the steps below.  

1. Download the libusb source from [here](https://libusb.info/) or the [libusb GitHub releases page](https://github.com/libusb/libusb/releases).
2. In the libusb source directory, open ```msvc/libusb_2017.sln``` with Visual Studio.
3. Take note of the configuration (e.g. Debug, x64), and change it if desired (aulon releases use Release, x86/Win32).
4. Highlight "libusb-1.0 (dll)" or "libusb-1.0 (static)" in the Solution Explorer.
5. Under the "Build" menu, click "Build libusb-1.0 ([dll/static])" (NOT "Build Solution").
6. In the main source directory, go into ```Win32/``` or ```x64/``` (depending on which configuration you picked).
7. Then, go into the ```Release/``` or ```Debug/``` directory, and finally into the ```dll/``` or ```static/``` directory (once again, all depending on the configuration you picked).
8. Take the ```.lib``` file. If you chose to dynamically link, you'll also want the ```.dll```.
9. Get the ```libusb.h``` file from ```libusb/``` in the main directory.

#### Part 2: Compiling aulon  

1. Put the ```.lib``` and ```libusb.h``` files into ```libusb/``` in the main aulon directory.
2. In the main directory, open ```build/msvc/aulon.sln``` with Visual Studio.
3. Change the configuration to match that of the libusb lib you compiled or downloaded.
4. Under the "Build" menu, click "Build Solution".
5. aulon.exe will be under ```bin/msvc/```. If you are dynamically linking, you'll need to put the ```libusb-1.0.dll``` in the same directory (or system32) to run aulon.

### Linux
Coming soon...  

