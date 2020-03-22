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
#include "console.h"

/* 临时为测试添加的 */
#include "ioqueue.h"
#include "keyboard.h"

void test_thread_1(void *arg);
void test_thread_2(void *arg);

//int _start(void)
int main(void)
{
    put_str("i am kernel 12\b3\n");

    init_all();
    // asm volatile("sti");    // 打开外部中断
    
    thread_start("consumer_thread_01", 31, test_thread_1, "\nconsumer_1\n");
    thread_start("consumer_thread_02", 8, test_thread_2, "\nthread_02\n");

    intr_enable();  // 打开中断，使时钟中断起作用
    
    // console_put_str("Main\n");
    // console_put_str("Main\n");
    
    while(1);
        // console_put_str("Main\n");
    return 0;
}


void test_thread_1(void *para)
{
    while(1)
    {
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&keyboard_buf))
        {
            console_put_str(para);
            char byte = ioq_getchar(&keyboard_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}

void test_thread_2(void *para)
{
    while(1)
    {
        enum intr_status old_status = intr_disable();
        if(!ioq_empty(&keyboard_buf))
        {
            console_put_str(para);
            char byte = ioq_getchar(&keyboard_buf);
            console_put_char(byte);
        }
        intr_set_status(old_status);
    }
}