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

#include "process.h"

void test_thread_1(void *arg);
void test_thread_2(void *arg);
void u_program_01(void);
void u_program_02(void);

int a = 0, b = 0;

//int _start(void)
int main(void)
{
    put_str("i am kernel 12\b3\n");

    init_all();


    // 用户进程
    process_execute(u_program_01, "\nprocess_1\n");
    process_execute(u_program_02, "\nprocess_02\n");

    
    // 内核线程
    thread_start("k_thread_01", 31, test_thread_1, "\nthread_1\n");
    thread_start("k_thread_02", 31, test_thread_2, "\nthread_02\n");

    intr_enable();  // 打开中断，使时钟中断起作用
    
    
    while(1);
    return 0;
}


void test_thread_1(void *para)
{
    // console_put_str("v_a:0x");
    // console_put_int(a);

    // console_put_str("v_a:0x");
    // console_put_int(a);

    // console_put_str("v_a:0x");
    // console_put_int(a);    
    while(1)
    {
        console_put_str("\nv_a:0x");
        console_put_int(a);
        console_put_char('\n');
    }
}

void test_thread_2(void *para)
{
    // console_put_str("v_b:0x");
    // console_put_int(b);

    // console_put_str("v_b:0x");
    // console_put_int(b);

    // console_put_str("v_b:0x");
    // console_put_int(b);    
    while(1)
    {
        console_put_str("\nv_b:0x");
        console_put_int(b);
        console_put_char('\n');
    }
}

void u_program_01(void)
{
    while(1)
        a++;
}

void u_program_02(void)
{
    while(1)
        b++;
}