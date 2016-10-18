#!/bin/bash

gcc -g -o attach_weight attach_weight.c
sort -t"	" -k1 -n hps.att > hps.att.srt
./attach_weight hps.att.srt input.txt hps.openfst
cp hps.openfst ../
