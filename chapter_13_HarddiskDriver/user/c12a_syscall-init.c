#include "syscall-init.h"
#include "syscall.h"

#include "thread.h"     // running_thread

#include "print.h"      // put_str
#include "console.h"    // console_put_str

#include "string.h"     // strlen

#include "memory.h"     // sys_malloc sys_free

// 最大支持的系统调用子功能个数
#define syscall_number  32

void *syscall_table[syscall_number];


/* 初始化系统调用 */
void syscall_init(void)
{
    put_str("syscall_init begin...");
    
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_WRITE]  = sys_write;
    syscall_table[SYS_MALLOC] = sys_malloc;
    syscall_table[SYS_FREE] = sys_free;
    
    // put_str("syscall_init done!\n");
    put_str(" done!\n");
}


/* 返回当前任务的pid */
unsigned int sys_getpid(void)
{
    return running_thread()->pid;
}

/* 打印字符串str(未实现文件系统前的版本) */
unsigned int sys_write(char *str)
{
    console_put_str(str);
    return strlen(str);
}


