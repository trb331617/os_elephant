// FILE: main.c
// TITLE: 
// DATE: 20200229
// USAGE: 
//  gcc -m32 -I lib/ -c -o main.o main.c
//  ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
//   dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc 

#include "print.h"
#include "kernel/init.h"

//int _start(void)
void main(void)
{
    put_str("i am kernel\n12\b3");

    init_all();
    asm volatile("sti");    // 打开外部中断
    
	while(1);
}
