// FILE: main.c
// TITLE: 
// DATE: 20200229
// USAGE: 
//  gcc -m32 -I lib/ -c -o main.o main.c
//  ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
//   dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc 

#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"

void test_thread_1(void *arg);
void test_thread_2(void *arg);

//int _start(void)
int main(void)
{
    put_str("i am kernel 12\b3\n");

    init_all();
    // asm volatile("sti");    // 打开外部中断
    
    thread_start("kernel_thread_01", 31, test_thread_1, "thread_1\n");
    thread_start("kernel_thread_02", 8, test_thread_2, "thread_02\n");

    intr_enable();  // 打开中断，使时钟中断起作用
    
    put_str("Main\n");
    put_str("Main\n");
    
    while(1);
    return 0;
}


void test_thread_1(void *arg)
{
    char *para = arg;
    put_str(para);
    put_str(para);
    
    while(1);
}

void test_thread_2(void *arg)
{
    char *para = arg;
    put_str(para);
    put_str(para);
    
    while(1);
}