// FILE: main.c
// TITLE: 
// DATE: 20200229
// USAGE: 
//  gcc -m32 -I lib/ -c -o main.o main.c
//  ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
//   dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc 

#include "print.h"
#include "init.h"
#include "memory.h"

//int _start(void)
int main(void)
{
    put_str("i am kernel 12\b3\n");

    init_all();
    // asm volatile("sti");    // 打开外部中断
    
    void *addr = get_kernel_pages(3);
    put_str("get_kernel_page start vaddr is 0x");
    put_int((unsigned int)addr);
    put_str("\n\n");
    
	while(1);
    return 0;
}
