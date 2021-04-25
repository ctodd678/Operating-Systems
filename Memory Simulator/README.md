Connor Todd

## Installation

Type "make" in shell to compile program with included makefile.

## LIMITATIONS 
-The maxium number of process is limited to 26 (for each letter of the alphabet).

-A process must have a memory size of atleast 1 megabyte, if it is less it will automatically
 be set to 1 in order to avoid errors.

-A process must also have a memory size less than or equal to 128 megabytes. If it it over it will automatically set
 the memory size of the process to 128MB in order to avoid errors. 

-The output numbers are not rounded or modified so they may differ slightly from the expected output (a few were off by 1 in my testing).

-Process IDs are limited to the letters A to Z (upper case). However, it does work with almost any character that is not '-' in my testing.

## Usage

Run second program by typing:
-"./holes {testfile} first" for first fit allocation
-"./holes {testfile} best" for best fit allocation
-"./holes {testfile} next" for next fit allocation
-"./holes {testfile} worst" for worst fit allocation
