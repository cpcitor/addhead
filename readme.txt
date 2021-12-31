ADDHEAD 
(c) Kevin Thacker,1999-2010

A utility to help cross-development of software for the Amstrad CPC 8-bit
computer.

This utility has three functions:
- add a AMSDOS header to a file. (use -a option)

- remove a AMSDOS header from a file. (use -r option)

- check length and report an error if it is larger than the size specified (use -c option)

- use -h option to use length defined in header rather than length of file 
on filesystem. Example: You have extracted a file using cpcxfs. The length is a multiple
of a sector in length, the file has a header and the actual length is shorter.
Use this option to cut it correctly.

- check a file doesn't exceed a specific max memory address
(use -c option and specify max address)


AMSDOS is the default disc operating system for the Amstrad which is active
after the computer has been switched-on. 

The 128-byte header defines the file-type, file load address, file length,
execution address and a checksum.

If a header is missing, AMSDOS will treat the file as text and 
the file can't be loaded using the CAS IN DIRECT function, and it is not possible
to load the file using BASIC's LOAD function without a error being returned.

If the header is present, CAS IN DIRECT and BASIC's LOAD function can be used.

If you are using a emulator for development then you can inject the file
into a disk image using a utility like CPCfs, CPCXfs or libdsk. 

If you are using a real computer then you can copy the file to the Amstrad
using a utility like 22disk, or transfer the data via the parallel cable.

Included in the archive are the source files, a makefile for Linux/Unix systems,
and a windows command-line executable.

When adding a header the following two options can be used to define 
parameters in the header:

-t <type>

set the type of the file defined by the header. <type> is "binary" or 
"basic".

-x <address>

set the execution address of the file (to be used for binary files to make 
them auto run when executed with: RUN"<filename>" from CPC's BASIC.

-s <address>

set the start address of the file. This is used when adding a header to a file 
or when setting the start address for checking file doesn't exceed max address.

-c <length>

returns an error if the length is greater than length (doesn't include header if it has one). Useful if you are writing 
a game that should fit into a limit space, this will produce an error if it exceeds that space.

-f 

forces the operation. If using "-r" to remove, it will warn but continue without returning an error code.
If using "-a" to add, it will add it even if it has an existing header.

-h 

when removing a header, and if one exists on the file, it uses the length stored in the header to decide the final length,
otherwise it uses the actual length minus the size of the header.

