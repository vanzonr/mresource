/* 
 * mresource_actions - file-based resource key allocator module
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

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <error.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "mresource_actions.h"

/* Functional parameters: */
#define POLL_INTERVAL       2 /* number of seconds between trying to get a key */
#define MAX_LINE_LEN     1024 /* maximum number of character per key           */
#define SIGNAL_CHAR       '!' /* initial character on a line if key is used    */
#define SIGNAL_CHAR_STR   "!" /* initial character on a line if key is used    */
#define DESIGNAL_CHAR     ' ' /* initial character on a line if key is unused  */
#define DESIGNAL_CHAR_STR " " /* initial character on a line if key is unused  */

/****************************************************************************/

static void fill_file_lock_controls(struct flock* set_lock,
                                    struct flock* unset_lock)
{
    /* Fill two 'struct flock's needed for file locking and unlocking */
    
    pid_t pid = getpid();
    
    set_lock->l_type     = F_WRLCK;
    set_lock->l_whence   = SEEK_SET;
    set_lock->l_start    = 0;
    set_lock->l_len      = 0;
    set_lock->l_pid      = pid; 
    unset_lock->l_type   = F_UNLCK;
    unset_lock->l_whence = SEEK_SET;
    unset_lock->l_start  = 0;
    unset_lock->l_len    = 0;
    unset_lock->l_pid    = pid;
    
} /* end fill_file_lock_controls() */

/****************************************************************************/

int obtain_resource(char* filename,
                    int   nkeys,
                    int   timeout,
                    int   polltime,
                    bool  verbose)
{
    /* Resource management routine to obtain a resource given a resource file */
  
    int exitcode = 0;
    int max_retries = timeout?((timeout+polltime-1)/polltime):INT_MAX;
  
    for (int i = 0; i < nkeys && exitcode == 0; i++) {

        FILE*        file;
        int          file_descriptor;
        size_t       file_pointer;
        char         line[MAX_LINE_LEN+1];
        char*        checkline;
        bool         repeat = false;
        int          retry_count = 0;
        struct flock set_lock;
        struct flock unset_lock;

        /* optionally report what is being done */    
        if (verbose) {
            error(0, 0,
                  "Info: Obtaining a resource key from file '%s' with a timeout of %d s.",
                  filename, timeout);
        }

        /* to avoid race conditions accessing the resource key file, use locks */
        fill_file_lock_controls(&set_lock, &unset_lock);

        /* try getting a key until time out */    
        do {
            file = fopen(filename, "r+");

            if (file != NULL) {

                file_descriptor = fileno(file);
                fcntl(file_descriptor, F_SETLKW, &set_lock);

                do {
                    file_pointer = ftell(file);
                    checkline = fgets(line, sizeof(line), file);
                } while ( checkline != NULL && !feof(file) && line[0] == SIGNAL_CHAR );
            
                if (feof(file)) {
                    if (retry_count < max_retries) {
                        sleep(polltime);
                        retry_count++;
                        repeat = true; 
                    } else {
                        exitcode = TIME_OUT;
                        repeat = false;
                    }
                } else {           
                    fseek(file, file_pointer, SEEK_SET);
                    fprintf(file, "%c", SIGNAL_CHAR);
                    printf("%s", line + 1);
                    exitcode = NO_ERROR;
                    repeat = false;
                }
            
                fcntl(file_descriptor, F_SETLK, &unset_lock);
                fclose(file);

            } else {
            
                exitcode = FILE_NOT_OPEN;
            
            }
        
        } while (repeat); /* keep polling if resources were not avaliable */

        if (verbose && exitcode == 0) {    
            error(0, 0,
                  "Info: Resource key obtained from file '%s': %s",
                  filename, line+1);
        }
    
    }

    return exitcode;

} /* end obtain_resource() */

/****************************************************************************/

int release_resource(char*  filename,
                     int    nkeys,
                     char** keys,
                     int    delay,
                     bool   verbose)
{
    /* Resource management routine to release nkeys 'keys' from resource file. */

    int    exitcode = NO_ERROR;
    FILE*  file = NULL;
    pid_t  pid;
    
    /* optionally report what is being done */
    if (verbose) { 
        if (delay > 0) {
            error(0, 0,
                  "Info: Releasing the following resource key(s) from file '%s' with a delay of %d s:",
                  filename, delay);
        } else {            
            error(0, 0,
                  "Info: Releasing the following resource key(s) from file '%s':",
                  filename);
        }
        for (int i = 0; i < nkeys; i++) {
            fprintf(stderr,"%s ", keys[i]);
        }
        fprintf(stderr,"\n");          
    }

    /* check that the file exists */
    file = fopen(filename, "r+");
    if (file == NULL) {
        return FILE_NOT_OPEN;
    } else {
        fclose(file);
    }    

    /* double fork to daemonize only if there's a delay */
    if (delay > 0) {
        pid = fork();
        if (pid < 0) {
            error(1, 0,
                  "fork error");
        } else if (pid == 0) {  /* pid == 0 means this is the forked child */
            pid = fork();
            if (pid < 0) {
                error(1, 0,
                      "fork error");
            } else if (pid > 0) {
                /* parent from second fork, i.e. first child */
                return NO_ERROR;       
            } else {
                ; /* We are the second child and will continue with the function */
            }
        } else {
            if (waitpid(pid, NULL, 0) != pid) { /* wait for first child */
                error(1, 0,
                      "waitpid error");
            }
            return NO_ERROR;
        }
        /* If we get here, we know that we are the daemonized process. */
    }

    /* delay (could be zero) */
    sleep(delay);

    /* return the resource keys to the pool, using file locks */

    file = fopen(filename, "r+");

    if (file != NULL) {

        struct flock set_lock;
        struct flock unset_lock;
        int file_descriptor;
        
        fill_file_lock_controls(&set_lock, &unset_lock);
        file_descriptor = fileno(file);
        fcntl(file_descriptor, F_SETLKW, &set_lock);

        for (int i = 0; i < nkeys; i++) {

            size_t       file_pointer = 0;
            char         line[MAX_LINE_LEN+1];
            char*        checkline;

            fseek(file, file_pointer, SEEK_SET); 

            do {
                file_pointer = ftell(file);
                checkline = fgets(line, sizeof(line), file);
                if (strlen(line) > 0 && line[strlen(line)-1] == '\n') {
                    line[strlen(line)-1]='\0';
                }
            } while (checkline != NULL && !feof(file) &&
                     (strcmp(line+1, keys[i]) != 0 || strncmp(line, SIGNAL_CHAR_STR, 1) != 0));
            if (feof(file)) {
                exitcode |= NOT_FOUND;
            } else {      
                fseek(file, file_pointer, SEEK_SET);
                fprintf(file, "%c", DESIGNAL_CHAR);
                if (verbose) {
                    error(0, 0,
                          "Info: Resource key %s made available again in file '%s'.",
                          keys[i], filename);
                }
                exitcode |= NO_ERROR;
            }
        }
        
        fcntl(file_descriptor, F_SETLK, &unset_lock);
        fclose(file);

    } else {

        exitcode = FILE_NOT_OPEN;
        
    }
  
  return exitcode;

} /* end release_resource() */

/****************************************************************************/

int create_resource_file(char*  filename,
                         int    argc,
                         char** argv,
                         bool   verbose) 
{
    /* Create a resource key file with argc keys given by argv. */
      
    FILE* f;
    int   i;

    /* optionally report what is being done */

    if (verbose) {        
        error(0, 0,
              "Creating resource key file '%s'.",
              filename);
    }
    
    f = fopen(filename,"w"); 

    if ( f != NULL ) {

        for (i = 0; i < argc; i++) {
            fprintf(f, "%c%s\n", DESIGNAL_CHAR, argv[i]);
        }

        fclose(f);

        return 0;

    } else {

        return 1;
    }

} /* end create_resource_file() */

/****************************************************************************/

int append_resource_file(char*  filename,
                         int    argc,
                         char** argv,
                         bool   verbose)
{
    /* Append possible keys to a resource file that could be in use already */

    FILE*   file;
    int     file_descriptor;
    int     exitcode;
    int     i;
    struct flock set_lock, unset_lock;

    /* optionally report what is being done */
    if (verbose) {        
        error(0, 0,
              "Appending keys to file '%s'.",
              filename);
    }

    /* to avoid race conditions accessing the resource key file, use locks */
    fill_file_lock_controls(&set_lock, &unset_lock);

    while (1) {
        file = fopen(filename, "a");

        if (file != NULL) {

            file_descriptor = fileno(file);
            fcntl(file_descriptor, F_SETLKW, &set_lock);
            
            for (i = 0; i < argc; i++) {
                fprintf(file, "%c%s\n", DESIGNAL_CHAR, argv[i]);
            }
            
            fcntl(file_descriptor, F_SETLK, &unset_lock);
            fclose(file);

            exitcode = NO_ERROR;

            break;

        } else {
            
            exitcode = FILE_NOT_OPEN;
            
        }
        
    }

    return exitcode;
    
} /* end append_resource_file() */

/****************************************************************************/
