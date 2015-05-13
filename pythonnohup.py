import os,sys


# Below is from: http://stackoverflow.com/questions/6011235/run-a-program-from-python-and-have-it-continue-to-run-after-the-script-is-kille
# (in the answer by 'edoloughlin')

def spawnDaemon(func):
    # do the UNIX double-fork magic, see Stevens' "Advanced 
    # Programming in the UNIX Environment" for details (ISBN 0201563177)
    try: 
        pid = os.fork() 
        if pid > 0:
            # parent process, return and keep running
            return
    except OSError, e:
        print >>sys.stderr, "fork #1 failed: %d (%s)" % (e.errno, e.strerror) 
        sys.exit(1)

    os.setsid()

    # do second fork
    try: 
        pid = os.fork() 
        if pid > 0:
            # exit from second parent
            sys.exit(0) 
    except OSError, e: 
        print >>sys.stderr, "fork #2 failed: %d (%s)" % (e.errno, e.strerror) 
        sys.exit(1)

    # do stuff
    func()

    # all done
    os._exit(os.EX_OK)


import time
def f():
    print "A: Going to sleep"
    time.sleep(60)
    print "B: Waking up after catnap"
    f=open("hi","w");
    print >>f, "B: Waking up after catnap"
    f.close()

spawnDaemon(f)
