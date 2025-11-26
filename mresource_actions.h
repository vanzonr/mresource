/* 
 * mresource_actions.h - header for file-based resource key allocator module
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
#ifndef MRESOURCEACTIONSH
#define MRESOURCEACTIONSH

#include <stdbool.h>

/* Common parameters: */
#define POLL_INTERVAL    2  /* number of seconds between trying to get a key */
#define MAX_LINE_LEN  1024  /* maximum number of character per key           */
#define SIGNAL_CHAR     '!' /* initial character on a line if key is used    */
#define SIGNAL_CHAR_STR "!" /* initial character on a line if key is used    */
#define SWITCH_CHAR     '-' /* initial character of a command line switch    */

/* Possible actions the program may perform: */
enum Mode { 
    OBTAIN = 1, 
    RELEASE, 
    SHOW_HELP, 
    CREATE,
    APPEND,
    ERROR 
};

/* Possible error codes of the program: */
enum ExitCodes { 
    NO_ERROR = 0,     /* exit code when all's well                           */
    FILE_NOT_OPEN,    /* exit code when file could not be opened             */
    NOT_FOUND,        /* exit code when a key could not be found             */
    ARGUMENT_ERROR,   /* exit code when called with too few arguments        */
    TIME_OUT          /* exit code when key was not obtained before timeout  */
};

/* Resource management routine to obtain a resource given a resource file: */
int obtain_resource(char* filename,
                    int   nkeys,
                    int   timeout,
                    int   polltime,
                    bool  verbose);

/* Resource management routine to release nkeys 'keys' from resource file: */
int release_resource(char*  filename,
                     int    nkeys,
                     char** keys,
                     int    delay,
                     bool   verbose);

/* Resource management routine to release nkeys 'keys' from resource file: */
int release_resource(char*  filename,
                     int    nkeys,
                     char** keys,
                     int    delay,
                     bool   verbose);

/* Create a resource key file with argc keys given by argv: */
int create_resource_file(char*  filename,
                         int    argc,
                         char** argv,
                         bool   verbose);

/* Append possible keys to a resource file that could be in use already: */
int append_resource_file(char*  filename,
                         int    argc,
                         char** argv,
                         bool   verbose);

      

#endif
