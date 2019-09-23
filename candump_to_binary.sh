#!/bin/bash

# STARTS WITH (000.000000)  can0  666   [4]  48 41 43 4B               'HACK'
# CONTINUES WITH  (000.002008)  can0  666   [8]  23 0C C5 0B C3 0B 01 01   '#.......'

filename=$1
outfilename="$filename.bin"

rm -f $outfilename

while read p; do 
    ID=$(echo -ne $p | gawk '{ print $3 }')
    DLC=$(echo -ne $p | gawk '{ print $4 }')
    BYTE0=$(echo -ne $p | gawk '{ print $5 }')
    BYTE1=$(echo -ne $p | gawk '{ print $6 }')
    BYTE2=$(echo -ne $p | gawk '{ print $7 }')
    BYTE3=$(echo -ne $p | gawk '{ print $8 }')
    BYTE4=$(echo -ne $p | gawk '{ print $9 }')
    BYTE5=$(echo -ne $p | gawk '{ print $10 }')
    BYTE6=$(echo -ne $p | gawk '{ print $11 }')
    BYTE7=$(echo -ne $p | gawk '{ print $12 }')
    if [ "$ID" == "666" ] && [ "$DLC" == "[8]" ]
    then
        echo -n -e \\x$BYTE0\\x$BYTE1\\x$BYTE2\\x$BYTE3\\x$BYTE4\\x$BYTE5\\x$BYTE6\\x$BYTE7 >> $outfilename
        
    fi
    echo $p
done < $filename
