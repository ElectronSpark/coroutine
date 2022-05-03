#!/bin/bash
for (( c=0; c<=20; c++ ))
do
    for (( d=0; d<=20; d++ ))
    do
        ./example/test.elf $(( 2**c ))
    done
done 
