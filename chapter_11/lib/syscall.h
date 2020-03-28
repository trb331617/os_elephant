#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H

enum SYSCALL_NR{
    SYS_GETPID;
};

unsigned int getpid(void);

#endif