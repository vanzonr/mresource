#!/bin/bash
#
# mrtest.sh - tests resource management with mresource
#
# Copyright (c)  2013-2016  Ramses van Zon
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions;
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
#
# -------------------------
# Explanation of the script
# -------------------------
#
# This script first creates a file with 5 resource keys, i.e., R1, R2,
# R3, R4, and R5.
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
MAXSLEEP=18
RESOURCEFILE=/dev/shm/resourcefile

# Create resource file
./mresource $RESOURCEFILE -c R{1..4}

# Set up N tasks
for ((task=0; task<$N; task++))
do
    let sec=$RANDOM%$MAXSLEEP
    resource_handle=$(./mresource $RESOURCEFILE -t $TIMEOUT)
    if [ $? == 0 ]
    then
        echo $task:$resource_handle:$sec
        nohup sh -c "sleep $sec && ./mresource $RESOURCEFILE $resource_handle" >/dev/null 2>&1 &
    else
        echo $task:no resource available
    fi
done
