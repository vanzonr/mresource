/* 
 * mresource  - file-based resource key allocator
 *
 * Copyright (c) 2013-2025  Ramses van Zon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mresource_actions.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <error.h>

/* Error messages: */
char ExitMsg[5][22] = { [NO_ERROR]       = "", 
                        [FILE_NOT_OPEN]  = "Could not open file", 
                        [NOT_FOUND]      = "Could not find key", 
                        [ARGUMENT_ERROR] = "Argument error", 
                        [TIME_OUT]       = "Time-out" };

/* Show help message: */
void show_help1()
{
    printf("\n"
           "mresource - file-based resource key allocator\n"
           "\n"
           "  Usage:\n"
           "\n"
           "    mresource -h\n"
           "    mresource FILE [-t TIME] [-p POLLTIME] [-n NUMKEYS] [-v]\n"
           "    mresource FILE KEY1 [KEY2 ....] [-d DELAY] [-v]\n"
           "    mresource FILE -c KEY1 [KEY2 ....] [-v]\n"
           "    mresource FILE -a KEY1 [KEY2 ....] [-v]\n"           
           "\n"
           "  When given a FILE but no key(s), mresource prints out\n"
           "  the next available resource in the file, and marks it\n"
           "  as used in the file. If no resource is available, it\n"
           "  waits for POLLTIME seconds before trying again.\n"
           "\n"
           "  POLLTIME is 2 seconds by default, but can be set with\n"
           "  the optional '-p POLLTIME' argument\n"
           "\n"
           "  With '-t TIME', mresource only tries for TIME seconds.\n"
           "  Without '-t TIME', mresource waits untill a resource is\n"
           "  available.\n "
           "\n"
           "  When given a FILE and a KEY, that resource key gets\n"
           "  unmarked in the file, after a DELAY seconds lag time.\n"
           "\n"
           "  Note that FILE should contain a list of resource keys.\n"
           "  The first character of each line is reserved to store the\n"
           "  allocation signal: when it is a space (as it should be \n"
           "  initially), then the resource is not reserved, when it \n"
           "  is an exclamation mark it is.\n"
           "\n"
           "  mresource can generate such a file when invoked with\n"
           "  FILE, '-c', and a list of one or more keys.\n"
           "\n"
           "  mresource can insert more keys into such a file when\n"
           "  invoked with FILE, '-a', and a list of one or more keys.\n"
           "\n"
           "  If '-v' is given, mresource will write out to stderr what\n"
           "  it is doing.\n"
           "\n"
           "  TIP: When accessed a lot, put FILE a ram-based file\n"
           "  system, e.g., /tmp or /dev/shm.\n"
           "\n\n"
           "Ramses van Zon, SciNet, Toronto, 2013-2025\n"
           "\n"); 
    
} /* end show_help() */

/* Read command line: */
void read_cmdline(int        argc,
                  char**     argv,
                  enum Mode* mode,
                  char**     file,
                  char***    keys,
                  int*       nkeys,
                  int*       timeout,
                  int*       delay,
                  int*       polltime,
                  bool*      repsyntax,
                  bool*      verbose) 
{    
    int argi;
    int n = 1;
    
    *file    = NULL;
    *keys    = NULL;
    *nkeys   = 0;
    *mode    = OBTAIN;
    *timeout = INT_MAX;
    *delay   = 0;
    *polltime= 2;
    *verbose = false;
    for (argi = 1; argi < argc; argi++) {
        if (argv[argi][0] == SWITCH_CHAR) {
            switch (argv[argi][1]) {
            case 'c': 
                *mode = CREATE;
                break;
            case 'a': 
                *mode = APPEND;
                break;
            case 'h': 
                if (argi == 1 && argc == 2) {
                    *mode = SHOW_HELP;
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Too many arguments; '-h' must be the only argument.");
                }
                break;
            case 't': 
                if (argi < argc-1) {
                    *timeout = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-t'.");
                }
                break;
            case 'd': 
                if (argi < argc-1) {
                    *delay = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-d'.");
                }
                break;
            case 'p': 
                if (argi < argc-1) {
                    *polltime = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-p'.");
                }
                break;
            case 'v': 
                *verbose = true;
                break;
            case 'r': 
                *repsyntax = true;
                error(ARGUMENT_ERROR, 0, "Repeated syntax is not yet supported.");
                break;
            case 'n': 
                if (argi < argc-1) {
                    n = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-p'.");
                }
                break;
            default:
                error(ARGUMENT_ERROR, 0,
                      "Unknown option '%s.'", argv[argi]);
            }
        } else {
            if (!*file) {
                *file = argv[argi];
            } else if (!*keys) {
                if (*mode == OBTAIN) {
                    *mode = RELEASE;
                }
                *keys = argv + argi;
                for (;argi < argc && argv[argi][0] != SWITCH_CHAR; argi++) {
                    (*nkeys)++;
                }
                argi--;
            } else { 
                error(ARGUMENT_ERROR, 0,
                      "Extraneous argument '%s'\n", argv[argi]);
            }
        }
    }
    /* the -n arguments sets nkeys iff in OBTAIN mode*/
    if (*mode == OBTAIN) {
        *nkeys = n; 
    }
} /* end read_cmdline() */

/* Show help message: */
void show_help()
{
    printf("\n"
           "mresource - file-based resource key allocator\n"
           "\n"
           "  Usage:\n"
           "\n"
           "    mresource [ -h | help ]\n"
           "    mresource get [-v] [-t TIME] [-p POLLTIME] [-n NUMKEYS] -f FILE\n"
           "    mresource put [-v] [-d DELAY] -f FILE KEY1 [KEY2 ....] \n"          
           "    mresource create [-v] -f FILE KEY1 [KEY2 ....] \n"
           "    mresource append [-v] -f FILE KEY1 [KEY2 ....]\n"
           "\n"
           "  FILE should contain a list of resource keys. The first character of\n"
           "  each line is reserved to store the allocation signal: when it is a \n"
           "  space (as it should be initially), the resource is not reserved, when\n"
           "  it is an exclamation mark it is.\n"
           "  (Tip: When used a lot, put this file on a ram-based file system.)\n"
           "\n"
           "  The 'get' subcommand prints out the next NUMKEYS available resources\n"
           "  from FILE, and marks them as used. If no resource is available, it\n"
           "  waits for POLLTIME seconds before trying again, for upto TIME seconds.\n"
           "  (defaults: NUMKEYS=1, POLLTIME=2, TIME=infinite)\n"
           "\n"
           "  The 'put' subcommand flags the given keys in FILE as available,\n"
           "  optionally after a DELAY seconds lag time (default is no delay).\n"
           "  If delayed, it spawns a temporary daemon and errors cannot be caught.\n"
           "\n"
           "  The 'create' subcommand generates a resource file with the KEYs given\n"
           "  on the command line.\n"
           "\n"
           "  The 'append' subcommand inserts more keys into the give resource file.\n"
           "\n"
           "  If '-v' is given, mresource will write out to stderr what it is doing.\n"
           "\n"
           "Ramses van Zon, SciNet, Toronto, 2013-2025\n"
           "\n"); 
    
} /* end show_help() */

/* Read command line: */
void read_cmdline2(int        argc,
                  char**     argv,
                  enum Mode* mode,
                  char**     file,
                  char***    keys,
                  int*       nkeys,
                  int*       timeout,
                  int*       delay,
                  int*       polltime,
                  bool*      repsyntax,
                  bool*      verbose) 
{    
    int argi;
    int n = 1;
    
    *file    = NULL;
    *keys    = NULL;
    *nkeys   = 0;
    *mode    = ERROR;   /* signals that no mode has been selected yet. */
    *timeout = INT_MAX; /* about 68 years, so effectively infinite timeout. */
    *delay   = 0;
    *polltime= 2;
    *verbose = false;
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "help")==0 || strcmp(argv[argi], "-h")==0) {
            *mode = SHOW_HELP;
        } else if (strcmp(argv[argi], "get")==0) {
            *mode = OBTAIN;
        } else if (strcmp(argv[argi], "put")==0) {
            *mode = RELEASE;
        } else if (strcmp(argv[argi], "create")==0) {
            *mode = CREATE;
        } else if (strcmp(argv[argi], "append")==0) {
            *mode = APPEND;
        } else if (argv[argi][0] == SWITCH_CHAR) {
            switch (argv[argi][1]) {
            case 'f':
                *file = argv[++argi];
                break;
            case 't': 
                if (argi < argc-1) {
                    *timeout = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-t'.");
                }
                break;
            case 'd': 
                if (argi < argc-1) {
                    *delay = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-d'.");
                }
                break;
            case 'p': 
                if (argi < argc-1) {
                    *polltime = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-p'.");
                }
                break;
            case 'v': 
                *verbose = true;
                break;
            case 'r': 
                *repsyntax = true;
                error(ARGUMENT_ERROR, 0, "Repeated syntax is not yet supported.");
                break;
            case 'n': 
                if (argi < argc-1) {
                    n = atoi(argv[++argi]);
                } else {
                    error(ARGUMENT_ERROR, 0,
                          "Missing parameter for '-p'.");
                }
                break;
            default:
                error(ARGUMENT_ERROR, 0,
                      "Unknown option '%s.'", argv[argi]);
            }
        } else {
            if (!*keys) {
                if (*mode == OBTAIN) {
                    *mode = RELEASE;
                }
                *keys = argv + argi;
                for (;argi < argc && argv[argi][0] != SWITCH_CHAR; argi++) {
                    (*nkeys)++;
                }
                argi--;
            } else { 
                error(ARGUMENT_ERROR, 0,
                      "Extraneous argument '%s'\n", argv[argi]);
            }
        }
    }
    /* the -n arguments sets nkeys iff in OBTAIN mode*/
    if (*mode == OBTAIN) {
        *nkeys = n; 
    }
} /* end read_cmdline() */


/* Program start: */
int main(int argc, char** argv) 
{    
    enum Mode  mode     = SHOW_HELP; /* what are we supposed to be doing?  */
    char*      filename = "";        /* file with resource names           */
    char**     keys     = NULL;      /* requested key(s)                   */
    int        timeout  = 0;         /* time-out delay                     */
    int        nkeys    = 0;         /* number of keys on command line     */
    int        delay    = 0;         /* delay in releasing the key         */
    int        polltime = 1;         /* number of seconds in between tries */
    bool       verbose  = false;     /* should there be info to stderr?    */
    bool       repsyntax= false;     /* collapse n repeated keys to key:n ?*/
    int        exitcode = 0;
    
    read_cmdline2(argc, argv,
                 &mode, &filename, &keys, &nkeys, &timeout, &delay, &polltime,
                 &repsyntax, &verbose);

    if (verbose) {
        error(0, 0,
              "mode:    \t%d\nfilename:\t%s\nnkeys:    \t%d\n",
              mode, filename, nkeys);
    }
    
    switch (mode) {
    case CREATE:
        exitcode = create_resource_file(filename, nkeys, keys, verbose); 
        break;
    case APPEND:
        exitcode = append_resource_file(filename, nkeys, keys, verbose); 
        break;
    case OBTAIN:
        exitcode = obtain_resource(filename, nkeys, timeout, polltime, verbose);
        break;
    case RELEASE:
        exitcode = release_resource(filename, nkeys, keys, delay, verbose);
        break;
    case SHOW_HELP:
        show_help();
        break;
    case ERROR:
        show_help();
        exitcode = 1;
    }

    if (exitcode != 0)  {
        error(exitcode, 0,
              "Error: %s.",
              ExitMsg[exitcode]);
    }

    return exitcode;
    
} /* end main() */

/****************************************************************************/
