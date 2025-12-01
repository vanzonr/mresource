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

/**
 * @brief Obtain resource keys from a resource file.
 * 
 * Tries to obtain nkeys from the specified file, with timeout and polling.
 * The function will print the obtained keys to stdout, each on a separate line.
 * 
 * @param filename Name of the resource file.
 * @param nkeys    Number of keys to obtain.
 * @param timeout  Timeout in seconds.
 * @param polltime Poll interval in seconds.
 * @param verbose  If true, enables verbose output.
 * @return Exit code (NO_ERROR, FILE_NOT_OPEN, TIME_OUT, etc.)
 */
int obtain_resource(char* filename,
                    int   nkeys,
                    int   timeout,
                    int   polltime,
                    bool  verbose);

/**
 * @brief Release resource keys back to the resource file.
 * 
 * Releases nkeys back to the specified file, optionally after a delay.
 * If delay is greater than zero, the function will fork a daemon process 
 * to handle the release. Any non-existing keys or already released keys
 * will trigger an error only if delay is zero.
 * 
 * @param filename Name of the resource file.
 * @param nkeys    Number of keys to release.
 * @param keys     Array of keys to release.
 * @param delay    Delay in seconds before releasing.
 * @param verbose  If true, enables verbose output.
 * @return Exit code (NO_ERROR, FILE_NOT_OPEN, NOT_FOUND, etc.)
 */                  
int release_resource(char*  filename,
                     int    nkeys,
                     char** keys,
                     int    delay,
                     bool   verbose);

/**
 * @brief Create a new resource key file with specified keys.
 * 
 * Creates a resource file and populates it with nkeys keys.
 * 
 * @param filename Name of the resource file to create.
 * @param nkeys    Number of keys.
 * @param keys     Array of key strings.
 * @param verbose  If true, enables verbose output.
 * @return Exit code (NO_ERROR, FILE_NOT_OPEN, etc.)
 */
int create_resource_file(char*  filename,
                         int    nkeys,
                         char** keys,
                         bool   verbose);

      
/**
 * @brief Append keys to an existing resource file.
 * 
 * Appends nkeys keys to the specified resource file.
 * 
 * @param filename Name of the resource file to append to.
 * @param nkeys    Number of keys.
 * @param keys     Array of key strings.
 * @param verbose  If true, enables verbose output.
 * @return Exit code (NO_ERROR, FILE_NOT_OPEN, etc.)
 */
int append_resource_file(char*  filename,
                         int    nkeys,
                         char** keys,
                         bool   verbose);

#endif
