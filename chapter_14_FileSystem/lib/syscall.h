#ifndef __LIB_SYSCALL_H
#define __LIB_SYSCALL_H

enum SYSCALL_NR{
    SYS_GETPID,
    SYS_WRITE,
    SYS_MALLOC,
    SYS_FREE,
};

unsigned int getpid(void);
// unsigned int write(char *str);

/* 把buf中count个字符写入文件描述符fd */
unsigned int write(signed int fd, const void *buf, unsigned int count);

void *malloc(unsigned int size);
void free(void *vaddr);

#endif