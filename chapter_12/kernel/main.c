// FILE: main.c
// TITLE: 
// DATE: 20200229
// USAGE: 
//  gcc -m32 -I lib/ -c -o main.o main.c
//  ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/print.o
//   dd if=kernel.bin of=hd60M.img bs=512 count=200 seek=9 conv=notrunc 

#include "print.h"
#include "console.h"

#include "init.h"
#include "interrupt.h"

#include "thread.h"
#include "process.h"

#include "syscall-init.h"
#include "syscall.h"

#include "stdio.h"      // printf

#include "memory.h"     // sys_malloc()

void k_thread_1(void *arg);
void k_thread_2(void *arg);
void u_program_01(void);
void u_program_02(void);

int process_a_pid = 0, process_b_pid = 0;

//int _start(void)
int main(void)
{
    put_str("i am kernel chapter_10\b2 =^_^=\n");

    init_all();
    intr_enable();  // 打开中断，使时钟中断起作用

    // 用户进程
    process_execute(u_program_01, "process_1\n");
    process_execute(u_program_02, "process_02\n");
    
    // 内核线程
    thread_start("kernel_thread_01", 31, k_thread_1, "i am thread_1\n");
    thread_start("kernel_thread_02", 31, k_thread_2, "i am thread_02\n");

    // 打印main线程的pid
    // console_put_str("  main pid: 0x");
    // console_put_int(sys_getpid());
    // console_put_char('\n');        

    while(1);
    return 0;
}


void k_thread_1(void *arg)
{
    // console_put_str("  I am k_thread_1, my pid: 0x");
    // console_put_int(sys_getpid());
    // console_put_char('\n');
    
    void *addr1, *addr2, *addr3;
    addr1 = sys_malloc(256);
    addr2 = sys_malloc(255);
    addr3 = sys_malloc(254);
    
    console_put_str("  k_thread_1 malloc addr: 0x");
    console_put_int((int)addr1);
    console_put_str(", ");
    console_put_int((int)addr2);
    console_put_str(", ");
    console_put_int((int)addr3);
    console_put_char('\n');
    
    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    
    // console_put_str("    k_thread_1 end");
    while(1);
}

void k_thread_2(void *arg)
{
    // console_put_str("  I am k_thread_2, my pid: 0x");
    // console_put_int(sys_getpid());
    // console_put_char('\n');
    
    void *addr1, *addr2, *addr3;
    addr1 = sys_malloc(256);
    addr2 = sys_malloc(255);
    addr3 = sys_malloc(254);
    
    console_put_str("  k_thread_2 malloc addr: 0x");
    console_put_int((int)addr1);
    console_put_str(", ");
    console_put_int((int)addr2);
    console_put_str(", ");
    console_put_int((int)addr3);
    console_put_char('\n');
    
    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    
    // console_put_str("    k_thread_2 end\n");
    while(1);
}

void u_program_01(void)
{
    char *name = "process_a"; 
    
    void* addr1 = malloc(256);
    printf("    i am %s, my pid: %d%c", name, getpid(), '\n');
    
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf("    prog_a malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
   
    while(1);
}

void u_program_02(void)
{
    char *name = "process_b";
    printf("    i am %s, my pid: %d%c", name, getpid(), '\n');
    
    void* addr1 = malloc(256);
    void* addr2 = malloc(255);
    void* addr3 = malloc(254);
    printf("    prog_b malloc addr:0x%x, 0x%x, 0x%x\n", (int)addr1, (int)addr2, (int)addr3);

    int cpu_delay = 100000;
    while(cpu_delay-- > 0);
    free(addr1);
    free(addr2);
    free(addr3);
   
    while(1);
}