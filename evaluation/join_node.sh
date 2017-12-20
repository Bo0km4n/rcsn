#!bin/sh
FILE_NAME=$1
export ALL_JOIN=`cat $FILE_NAME | grep -c 'level: *'`
export LEVEL_1=`cat $FILE_NAME | grep -c 'level: 1'`
export LEVEL_2=`cat $FILE_NAME | grep -c 'level: 2'`
export LEVEL_3=`cat $FILE_NAME | grep -c 'level: 3'`
echo all ring node : $ALL_JOIN 
echo level_1: $LEVEL_1
echo level_2: $LEVEL_2
echo level_3: $LEVEL_3
