#include "syscall.h"

// 大括号代码块
// 大括号中最后一个语句的值会作为大括号代码块的返回值

/* 无参数的系统调用 */
#define _syscall0(NUMBER) ({    \
    int ret;                    \
    asm volatile( "int $0x80"   \
    : "=a" (ret)                \
    : "a" (NUMBER)              \
    : "memory"                  \
    );                          \
    ret;                        \
})


/* 一个参数的系统调用 */
#define _syscall1(NUMBER, ARG1) ({  \
    int ret;                    \
    asm volatile( "int $0x80"   \
    : "=a" (ret)                \
    : "a" (NUMBER), "b" (ARG1)  \
    : "memory"                  \
    );                          \
    ret;                        \
})

/* 两个参数的系统调用 */
#define _syscall2(NUMBER, ARG1, ARG2) ({    \
    int ret;                    \
    asm volatile( "int $0x80"   \
    : "=a" (ret)                \
    : "a" (NUMBER), "b" (ARG1), "c" (ARG2)  \
    : "memory"                  \
    );                          \
    ret;                        \
})

/* 三个参数的系统调用 */
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({   \
    int ret;                    \
    asm volatile( "int $0x80"   \
    : "=a" (ret)                \
    : "a" (NUMBER), "b" (ARG1), "c" (ARG2), "d" (ARG3) \
    : "memory"                  \
    );                          \
    ret;                        \
})


/*** 系统调用之栈传递参数 ***/
/* 用户程序需要在执行int 0x80之前，将参数和子功能号压入用户栈
 * 这里约定下: 先压入参数, 再压入子功能号
 */
 
/* 无参数的系统调用  之栈传递参数版本 */
/***
#define _syscall0_stack(NUMBER) ({ \
    int ret; \
    asm volatile( "pushl %[number]; int $0x80; addl $4, %%esp" \
    : "=a" (ret)            \
    : [number] "i" (NUMBER) \
    : "memory"              \
    );                      \
    ret;                    \
})
***/

/* 三个参数的系统调用 之栈传递参数版本 */
/***
#define _syscall3_stack(NUMBER, ARG0, ARG1, ARG2) ({ \
    int ret;        \
    asm volatile (  \
    "pushl %[arg2]; pushl %[arg1]; pushl %[arg0];"  \
    "pushl %[number]; int $0x80; addl $16, %%esp"   \
    : "=a" (ret)                                    \
    : [number] "i" (NUMBER), [arg0] "g" (ARG0),     \
      [arg1] "g" (ARG1), [arg2] "g" (ARG2)          \
    : "memory"      \
    );              \
    ret;            \
})
***/

/* 返回当前任务pid */
unsigned int getpid()
{
    // 测试栈传递参数的版本
    // return _syscall0_stack(SYS_GETPID);
    
    return _syscall0(SYS_GETPID);
}

// unsigned int write(char *str)
// {
    // return _syscall1(SYS_WRITE, str);
// }


/* 把buf中count个字符写入文件描述符fd */
unsigned int write(signed int fd, const void *buf, unsigned int count)
{
    return _syscall3(SYS_WRITE, fd, buf, count);
}


/* 申请size字节大小的内存, 并返回内存首地址 */
void *malloc(unsigned int size)
{
    return (void *)_syscall1(SYS_MALLOC, size);
}

/* 释放vaddr指向的内存 */
void free(void *vaddr)
{
    _syscall1(SYS_FREE, vaddr);
}
