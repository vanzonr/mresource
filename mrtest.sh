#!/bin/bash
# mrtest - tests resource management

# create resource file

cat > resourcefile <<EOF
 R1
 R2
 R3
 R4
 R5
EOF

# set up N tasks

N=40
TIMEOUT=1
MAXSLEEP=15

for ((i=0;i<$N;i++)); do
    res=$(mresource resourcefile -t $TIMEOUT)
    if [ $? == 0 ]; then
        let sec=$RANDOM%$MAXSLEEP
        echo "$i:$res"
        (sleep $sec; mresource resourcefile $res) &
    else
        echo "$i:<no resource>"
    fi
done
