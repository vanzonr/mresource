mresource - file-based resource management
==========================================

mresource is linux program to get and release resources using a
file to keep track of which resources are locked and which are
available.

mresource is open-source, and is released under the MIT license. This
library is distributed WITHOUT ANY WARRANTY. For details, see the file
named 'LICENSE', and license statements in the source files.

Installation
============

mresource is written in c and only works on tested on linux. 
Compilation can be done using the provided Makefile (type 'make'), 
or by simply compiling the source file mresource.c.  Copying the 
'mresource' executable to a location within the PATH is enough
to install it.

Documentation
=============

'mresource -h' gives a short description of how to use mresource.
There is currently no further documentation.

Reporting Bugs
==============

Under the license, you are not required to report bugs, but are
encouraged to do so.  If you find a bug, you may report it to
rzon@scinet.utoronto.ca or vanzonr@gmail.com. 

Files
=====

mresource.c:        The application, written in C.

Makefile:           Compilation make file. Use 'make'.

mrtest.sh:          A bash script to test mresource. 
                    Can also be run using 'make test'.

WARRANTEE:          File that expresses that there is no warrantee

LICENSE:            Text of the MIT license

AUTHOR:             Name and email address of the author

README.md:          This file

Ramses van Zon - 12 May 2016 
