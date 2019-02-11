#!/bin/bash

dom=1

function chkdomain ()
{
    cleos  -u http://localhost:8889 get table fio.system fio.system domains | grep -q brd
    dom=$?
}

count=0
chkdomain
while [ $dom -eq 0 ]; do
    count=$(($count + 1))
    if [ $(($count % 60 )) -eq 0 ]; then
        echo domain still good after $count seconds
    fi
    sleep 1
    chkdomain
done

echo domain gone after $count seconds
