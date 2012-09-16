#!/bin/bash

MCU=msp430g2231
FILE=omd-lapse
rm $FILE-$MCU.elf
msp430-gcc -Os -Wall -g -mmcu=$MCU -o $FILE-$MCU.elf $FILE.c
sudo mspdebug rf2500 "prog $FILE-$MCU.elf"
