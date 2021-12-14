### Command-line options
aulon can be made to run commands from a text file rather than from standard input. To do this, use the ```-f [command file]``` argument on the command line. Each command should be on a separate line.  
If you are using a [logging build](https://github.com/jbop1626/aulon/blob/master/src/defs.h), you can specify a log file with the command line argument ```-l [log file]```.  

### Commands
#### Normal  
```B```
Initiate a connection to the console. Run this before any other commands.  
```I```
Request and print the console's unique identification number.  
```H value```
Flash the console's LED.  
```S hash_file```
Send a hash to the console for it to be signed with the console's private ECC key. The hash is read from ```hash_file```, and the resulting signature is printed to stdout.  
```J```
Set the console's clock to your PC's current time.  
```L```
List all files currently on the console.  
```F```
Dump the current filesystem block to ```current_fs.bin```.  
```1```
Dump the console's NAND to files on your PC. It will be saved to ```nand.bin``` and ```spare.bin``` in the current working directory.  
```X blk_num```
Read one block and its spare data from the console to files.  
```2```(\*)
Write a full NAND to the console. This operation overwrites the SKSA {(the iQue Player OS)} area of the iQue Player's NAND, which makes it an **unsafe** operation! Use this command *only* if you need to. The files ```nand.bin``` and ```spare.bin``` will need to be in the current working directory.  
```W```(\*)
Write a partial NAND to the console. This overwrites all of the NAND *except* the SKSA area (in other words all files/filesystem are overwritten, but not the OS). Most of the time, this should be the preferred way to copy a NAND to the player, because it is safer than a full overwrite as well as faster. The files ```nand.bin``` and ```spare.bin``` will need to be in the current working directory.  
```Y blk_num```(\*)
Write one block to the console from ```block_[blk_num].bin```.  
```3 file```
Read [file] from the console.  
```4 file```(\*)
Write [file] to the console.  
```R file```(\*)
Delete [file] from the console.  
```Q```
Close an open connection to the console.  

#### Miscellaneous
```h```
List commands.  
```?```
List copyright and licensing information.  
```q```
Quit aulon.  

\* Available only if writing is explicitly enabled in [defs.h](https://github.com/jbop1626/aulon/blob/master/src/defs.h).
