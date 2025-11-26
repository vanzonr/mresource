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

/*****************************************************************************/

#define POLL_INTERVAL    2  /* number of seconds between trying to get a key */
#define MAX_LINE_LEN  1024  /* maximum number of character per key           */
#define SIGNAL_CHAR     '!' /* initial character on a line if key is used    */
#define SWITCH_CHAR     '-' /* initial character of a command line switch    */

/*****************************************************************************/

/* Possible error codes of the program: */
enum ExitCodes { 
    NO_ERROR = 0,     /* exit code when all's well                           */
    FILE_NOT_OPEN,    /* exit code when file could not be opened             */
    NOT_FOUND,        /* exit code when a key could not be found             */
    ARGUMENT_ERROR,   /* exit code when called with too few arguments        */
    TIME_OUT          /* exit code when key was not obtained before timeout  */
};

/* Corresponding error messages: */
char ExitMsg[5][22] = { "", 
                        "Could not open file", 
                        "Could not find key", 
                        "Argument error", 
                        "Time-out" };

/*****************************************************************************/

/* Possible actions the program may perform */
enum Mode { 
    OBTAIN = 1, 
    RELEASE, 
    SHOW_HELP, 
    CREATE,
    APPEND,
    ERROR 
};

/*****************************************************************************/

static void read_cmdline(int        argc,
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
    /* Read command line */
    
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

/****************************************************************************/

static void show_help()
{
    /* Show help message */
    
    printf("\n"
           "mresource - file-based resource key allocator\n"
           "\n"
           "  Usage:\n"
           "\n"
           "    mresource [ -h | --help ]\n"
           "    mresource FILE [-t TIME] [-p POLLTIME] [-n NUMKEYS] [-r] [-v]\n"
           "    mresource FILE KEY [KEY2 ....] [-d DELAY] [-r] [-v]\n"
           "    mresource FILE -c KEY1 [KEY2 ....] [-r] [-v]\n"
           "    mresource FILE -a KEY1 [KEY2 ....] [-r] [-v]\n"
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
           "  If '-r' is given, the syntax 'key:n' will be used and\n"
           "  understood for repeated keys.\n"
           "\n"
           "  If '-v' is given, mresource will write out to stderr what\n"
           "  it is doing.\n"
           "\n"
           "  TIP: When accessed a lot, put FILE a ram-based file\n"
           "  system, e.g., /tmp or /dev/shm.\n"
           "\n\n"
           "Ramses van Zon, SciNet, Toronto, 2013-2022\n"
           "\n"); 
    
} /* end show_help() */

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

static int obtain_resource(char* filename,
                           int   timeout,
                           int   polltime,
                           bool  verbose)
{
    /* Resource management routine to obtain a resource given a resource file */
    
    FILE*        file;
    int          file_descriptor;
    size_t       file_pointer;
    char         line[MAX_LINE_LEN+1];
    char*        checkline;
    int          repeat = 0;
    int          exitcode = 0;
    int          rep = 0;
    int          maxrep = timeout?((timeout+polltime-1)/polltime):INT_MAX;
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
                if (rep < maxrep) {
                    sleep(polltime);
                    rep++;
                    repeat = 1; 
                } else {
                    exitcode = TIME_OUT;
                    repeat = 0;
                }
            } else {           
                fseek(file, file_pointer, SEEK_SET);
                fprintf(file, "%c", SIGNAL_CHAR);
                printf("%s", line + 1);
                exitcode = NO_ERROR;
                repeat = 0;
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
        
    return exitcode;

} /* end obtain_resource() */

/****************************************************************************/

static int release_resource(char* filename, char* key, int delay, bool verbose)
{
    /* Resource management routine to release 'key' from resource file */

    FILE*        file;
    int          file_descriptor;
    size_t       file_pointer;
    char         line[MAX_LINE_LEN+1];
    char*        checkline;
    int          exitcode;
    pid_t        pid;
    struct flock set_lock;
    struct flock unset_lock;

    /* optionally report what is being done */

    if (verbose) {        
        if (delay > 0) {
            error(0, 0,
                  "Info: Releasing the resource key %s from file '%s' with a delay of %d s.",
                  key, filename, delay);            
        } else {            
            error(0, 0,
                  "Info: Releasing the resource key %s from file '%s'.",
                  key, filename);
        }
    }

    /* quick check before delaying */

    file = fopen(filename, "r+");
    
    if (file == NULL) {
        return FILE_NOT_OPEN;
    } else {
        fclose(file);
    }

    /* double fork to daemonize only if there's a delay*/
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

    /* return the resource to the pool, using file locks */

    fill_file_lock_controls(&set_lock, &unset_lock);

    file = fopen(filename, "r+");

    if (file != NULL) {

        file_descriptor = fileno(file);
        fcntl(file_descriptor, F_SETLKW, &set_lock);
        
        do {
            file_pointer = ftell(file);
            checkline = fgets(line, sizeof(line), file);
            if (strlen(line) > 0 && line[strlen(line)-1] == '\n') {
                line[strlen(line)-1]='\0';
            }
        } while ( checkline != NULL && !feof(file) && strcmp(line+1, key) != 0 );
        
        if (feof(file)) {
            exitcode = NOT_FOUND;
        } else {           
            fseek(file, file_pointer, SEEK_SET);
            fprintf(file, "%c", ' ');
            if (verbose) {
                error(0, 0,
                      "Info: Resource key %s made available again in file '%s'.",
                      key, filename);
            }
            exitcode = NO_ERROR;
        }
        
        fcntl(file_descriptor, F_SETLK, &unset_lock);
        fclose(file);

    } else {

        exitcode = FILE_NOT_OPEN;
        
    }

    return exitcode;

} /* end release_resource() */

/****************************************************************************/

static int create_resource_file(char* filename, int argc, char**argv, bool verbose) 
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
            fprintf(f, " %s\n", argv[i]);
        }

        fclose(f);

        return 0;

    } else {

        return 1;
    }

} /* end create_resource_file() */

/****************************************************************************/

static int append_resource_file(char* filename, int argc, char**argv, bool verbose)
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
                fprintf(file, " %s\n", argv[i]);
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

int main(int argc, char**argv) 
{
    /* Main program */
    
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
    
    read_cmdline(argc, argv,
                 &mode, &filename, &keys, &nkeys, &timeout, &delay, &polltime,
                 &repsyntax,
                 &verbose);

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
        for (int i = 0; i < nkeys && exitcode == 0; i++) {
            exitcode = obtain_resource(filename, timeout, polltime, verbose);
        }
        break;
    case RELEASE:
        if (nkeys > 1) {
            delay = 0; /* delay does not work yet with multiple releases */
        }
        for (int i = 0; i < nkeys && exitcode == 0; i++) {
            exitcode = release_resource(filename, keys[i], delay, verbose);
        }
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
