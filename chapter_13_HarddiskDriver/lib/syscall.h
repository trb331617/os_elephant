#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
};

unsigned int getpid(void);
unsigned int write(char *str);

void *malloc(unsigned int size);
void free(void *vaddr);

#endif