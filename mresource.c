/* 
 * mresource  - file-based resource key allocator
 *
 * Ramses van Zon, October 2013
 * Second version, May 2015
 */

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <error.h>

/*****************************************************************************/

#define POLL_INTERVAL    2  /* number of seconds between trying to get a key */
#define MAX_LINE_LEN  1024  /* maximum number of character per key           */
#define SIGNAL_CHAR     '!' /* initial character on a line if key is used    */

/*****************************************************************************/

enum ExitCodes { 
    /* possible error codes of the program */
    NO_ERROR=0,       /* exit code when all's well                           */
    FILE_NOT_OPEN,    /* exit code when file could not be opened             */
    NOT_FOUND,        /* exit code when a key could not be found             */
    ARGUMENT_ERROR,   /* exit code when called with too few arguments        */
    TIME_OUT          /* exit code when key was not obtained before timeout  */
};

/*****************************************************************************/

enum Mode { 
    /* possible actions the program may perform */
    OBTAIN = 1, 
    RELEASE, 
    SHOW_HELP, 
    CREATE,
    ERROR 
};

/*****************************************************************************/

void read_cmdline(int argc, char**argv, enum Mode*mode, char**file, char***keys, int*nkeys, int* timeout, int* delay) 
{
    /* Read command line */
    *file    = NULL;
    *keys    = NULL;
    *nkeys   = 0;
    *mode    = OBTAIN;
    *timeout = INT_MAX;
    *delay   = 0;
    int argi;
    for (argi = 1; argi < argc; argi++) {
        if (argv[argi][0] == '-') {
            switch (argv[argi][1]) {
            case 'c': 
                *mode=CREATE;
                break;
            case 'h': 
                if (argi == 1 && argc == 2) 
                    *mode=SHOW_HELP;
                else
                    error(ARGUMENT_ERROR, 0, "Too many arguments; '-h' must be the only argument.");
                break;
            case 't': 
                if (argi < argc-1) 
                    *timeout = atoi(argv[++argi]);
                else
                    error(ARGUMENT_ERROR, 0, "Missing parameter for '-t'.");
                break;
            case 'd': 
                if (argi < argc-1) 
                    *delay = atoi(argv[++argi]);
                else
                    error(ARGUMENT_ERROR, 0, "Missing parameter for '-d'.");
                break;
            default:
                error(ARGUMENT_ERROR, 0, "Unknown option '%s.'", argv[argi]);
            }
        } else {
            if (!*file) 
                *file = argv[argi];
            else if (!*keys) {
                if (*mode == OBTAIN)
                    *mode = RELEASE;
                *keys = argv + argi;
                for (;argi < argc && argv[argi][0] != '-'; argi++) 
                    (*nkeys)++;
                argi--;
            } else 
                error(ARGUMENT_ERROR, 0, "Extraneous argument '%s'\n", argv[argi]);
        }
    }
} /* end read_cmdline */

/****************************************************************************/

void show_help()
{
    /* Show help message */
    printf("\n"
           "mresource - file-based resource key allocator\n"
           "\n"
           "Usage:\n"
           "\n"
           "    mresource [ -h | --help ]\n"
           "    mresource FILE [-t TIME] \n"
           "    mresource FILE [KEY] \n"
           "    mresource FILE -c KEY1 [KEY2 ....] \n"
           "\n"
           "  When given a FILE but no key(s), mresource prints out\n"
           "  the next available resource in the file, and marks it\n"
           "  as used in the file.\n"
           "\n"
           "  With '-t TIME', mresource only tries for TIME seconds.\n"
           "\n"
           "  When given a FILE and a KEY, that resource key gets\n"
           "  unmarked in the file.\n"
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
           "  TIP: When accessed a lot, put FILE a ram-based file\n"
           "  system, e.g., /tmp or /dev/shm.\n"
           "\n\n"
           "Ramses van Zon, SciNet, Toronto, 2013-2015\n"
           "\n"); 
    
} /* end show_help() */

/****************************************************************************/

void fill_file_lock_controls(struct flock* set_lock, struct flock* unset_lock)
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

} /* end fill_file_lock_controls(set_lock,unset_lock) */

/****************************************************************************/

int obtain_resource(char* filename, int timeout)
{
    /* Resource management routine to obtain a resource given a resource file */

    FILE*   file;
    int     file_descriptor;
    size_t  file_pointer;
    char    line[MAX_LINE_LEN+1];
    int     repeat;
    int     exitcode;
    struct flock set_lock, unset_lock;
    int     rep = 0;
    int     maxrep = timeout?((timeout+POLL_INTERVAL-1)/POLL_INTERVAL):INT_MAX;

    fill_file_lock_controls(&set_lock, &unset_lock);

    do {
        file = fopen(filename, "r+");

        if (file != NULL) {

            file_descriptor = fileno(file);
            fcntl(file_descriptor, F_SETLKW, &set_lock);

            do {
                file_pointer = ftell(file);
                fgets(line, sizeof(line), file);
            } while ( !feof(file) && line[0] == SIGNAL_CHAR );
            
            if (feof(file)) {
                if (rep<maxrep) {
                    sleep(POLL_INTERVAL);
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

    return exitcode;

} /* end obtain_resource(filename,timeout) */

/****************************************************************************/

int release_resource(char* filename, char* key)
{
    /* Resource management routine to release 'key' from resource file */

    FILE*   file;
    int     file_descriptor;
    size_t  file_pointer;
    char    line[MAX_LINE_LEN+1];
    int     exitcode;
    struct flock set_lock, unset_lock;

    fill_file_lock_controls(&set_lock, &unset_lock);


    file = fopen(filename, "r+");

    if (file != NULL) {

        file_descriptor = fileno(file);
        fcntl(file_descriptor, F_SETLKW, &set_lock);
        
        do {
            file_pointer = ftell(file);
            fgets(line, sizeof(line), file);
            if (line[strlen(line)-1]=='\n')
                line[strlen(line)-1]='\0';
        } while ( !feof(file) && strcmp(line+1, key) != 0 );
        
        if (feof(file))
            exitcode = NOT_FOUND;
        else {           
            fseek(file, file_pointer, SEEK_SET);
            fprintf(file, "%c", ' ');
            exitcode = NO_ERROR;
        }
        
        fcntl(file_descriptor, F_SETLK, &unset_lock);
        fclose(file);

    } else {

        exitcode = FILE_NOT_OPEN;
        
    }

    return exitcode;

} /* end release_resource(filename,key) */

/****************************************************************************/

int create_resource_file(char* filename, int argc, char**argv) 
{
    FILE* f = fopen(filename,"w");

    if ( f != NULL ) {

        int i;

        for (i=0; i< argc; i++) 
            fprintf(f, " %s\n", argv[i]);

        fclose(f);

        return 0;

    } else {

        return 1;
    }

} /* end release_resource */

/****************************************************************************/

int main(int argc, char**argv) 
{
    /* Main program */

    enum Mode  mode;        /* what are we supposed to be doing?  */
    char*      filename;    /* file with resource names           */
    char**     keys=NULL;   /* requested key(s)                   */
    int        timeout=0;   /* time-out delay                     */
    int        nkeys;       /* number of keys on command line     */
    int        delay;       /* delay in releasing the key (not yet implemented, but accepted from cmdline */

    read_cmdline(argc, argv, &mode, &filename, &keys, &nkeys, &timeout, &delay);

    switch (mode) {

      case CREATE:    return create_resource_file(filename, nkeys, keys);
      case OBTAIN:    return obtain_resource(filename, timeout);
      case RELEASE:   return release_resource(filename, keys[0]);
      case SHOW_HELP: show_help(); return 0;

    }

} /* end main((int argc, char**argv) */

/****************************************************************************/
