#!/bin/bash
#
# mrtest.sh - tests resource management
#
# It first creates a file with 5 resource keys, i.e., R1, R2, R3, R4, and R5.
#
# It then loops over 100 number, grabbing and locking a resource from
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
RESOURCEFILE=resourcefile

# create resource file

cat > $RESOURCEFILE <<EOF
 R1
 R2
 R3
 R4
 R5
EOF

# a future version of mresource will be able to generate this thusly:
# ./mresource $RESOURCEFILE -c R1 R2 R3 R4 R5

# set up N tasks


for ((i=0;i<$N;i++))
do
    resource_handle=$(./mresource $RESOURCEFILE -t $TIMEOUT)
    if [ $? == 0 ]; then
        let sec=$RANDOM%$MAXSLEEP
        echo $i:$resource_handle
        (sleep $sec; ./mresource $RESOURCEFILE $resource_handle) &
    else
        echo $i:no resource
    fi
done
