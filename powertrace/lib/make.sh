#!bin/sh
msp430-gcc -mmcu=msp430f1611 -c sha1.c -o sha1.o
msp430-gcc -mmcu=msp430f1611 -c l_bit.c -o l_bit.o
msp430-ar -cvq libsha1.a sha1.o
msp430-ar -cvq liblbit.a l_bit.o
