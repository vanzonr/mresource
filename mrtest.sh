#!/bin/bash
#
# mrtest.sh - tests resource management
#
# It first creates a file with 5 resource keys, i.e., R1, R2, R3, R4, and R5.
#
# It then loops over N numbers, grabbing and locking a resource from
# this file, and lauching a background process that will eventually
# release the lock after a random number of seconds.
#
# Because the grab-and-lock is only allowed to wait for a specified
# time, occasionally a resource is not grabbed, which is reported as
# 'no resource'
#
# Ramses van Zon, May 2015
#

# Parameters
N=40
TIMEOUT=1
MAXSLEEP=15
RESOURCEFILE=/dev/shm/resourcefile

# Create resource file
./mresource $RESOURCEFILE -c R{1..5}

# Set up N tasks
for ((task=0; task<$N; task++))
do
    resource_handle=$(./mresource $RESOURCEFILE -t $TIMEOUT)
    if [ $? == 0 ]
    then
        let sec=$RANDOM%$MAXSLEEP
        echo $task:$resource_handle
        nohup sh -c "sleep $sec && ./mresource $RESOURCEFILE $resource_handle" >/dev/null 2>&1 &
    else
        echo $task:no resource available
    fi
done
