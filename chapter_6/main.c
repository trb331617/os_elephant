// FILE: main.c
// TITLE: 
// DATE: 20200229
// USAGE: 
//  gcc -m32 -I lib/ -c -o main.o main.c
//  ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
//   dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc 

#include "print.h"

//int _start(void)
void main(void)
{
    put_str("i am kernel\n12\b3");
    put_int(0);
    put_char('\n');
    put_int('0');
    put_char('\n');
    put_int(88);
    put_char('\n');
    put_int(0x1234ABCD);
    put_char('\n');
    put_int(0x00000000);
    put_char('\n');
    
	while(1);
}
