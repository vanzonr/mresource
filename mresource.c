/* 
 * mresource  - file-based resource key allocator
 *
 * Ramses van Zon, October 2013
 */

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>

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
    ERROR 
};

/*****************************************************************************/

int read_cmdline(int argc, char**argv, enum Mode*mode, char**file, char**key) 
{
    /* Read command line */

    switch (argc) {
    
      case 0:
      case 1:          
          *mode = SHOW_HELP;
          return ARGUMENT_ERROR;
      case 2:
          if (argv[1][0] == '-') {
              *mode = SHOW_HELP;
              if (strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help") == 0) {
                  return NO_ERROR;
              } else {
                  return ARGUMENT_ERROR;
              }
          } else {
              *mode = OBTAIN;
              *file = argv[1];
              *key  = NULL;
              return NO_ERROR;
          }
      default:
          if (argv[2][0]=='-' && argv[2][1]=='t') {
              /* time-out*/
              *mode = OBTAIN;
              *file = argv[1];
              *key = argv[3];
              return NO_ERROR;
          } else {
              *mode = RELEASE;
              *file = argv[1];
              *key  = argv[2];
              return NO_ERROR;
          }
    }

    return NO_ERROR;

} /* end read_cmdline(argc,argv,mode,file,key) */

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
           "\n"
           "  When given a FILE, mresource checks out the next\n"
           "  available resource in the file, and marks it as used\n"
           "  in the file.\n\n"
           "  With '-t TIME', mresource ony tries for TIME seconds.\n"
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
           "  TIP: When accessed a lot, put FILE a ram-based file\n"
           "  system, e.g., /tmp or /dev/shm.\n"
           "\n\n"
           "Ramses van Zon, SciNet, Toronto, October 2013\n"
           "\n"); 
    
} /* end show_help() */

/****************************************************************************/

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

int obtain_resource(char* filename, char* timeout)
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
    int     maxrep = timeout?((atoi(timeout)+POLL_INTERVAL-1)/POLL_INTERVAL):INT_MAX;

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

} /* end obtain_resource(filename) */

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

int main(int argc, char**argv) 
{
    /* Main program */

    enum Mode  mode;      /* what are we supposed to be doing?  */
    char*      filename;  /* file with resource names           */
    char*      key;       /* requested key                      */

    if (read_cmdline(argc, argv, &mode, &filename, &key)) {
        if (mode==SHOW_HELP) show_help();
        return ARGUMENT_ERROR;
    }
    /* if command arguments were o.k., do one of the following: */

    switch(mode) {

      case OBTAIN:    {char* timeout=key;return obtain_resource(filename, timeout);}
      case RELEASE:   return release_resource(filename, key);
      case SHOW_HELP: show_help(); return 0;

    }

} /* end main((int argc, char**argv) */

/****************************************************************************/
