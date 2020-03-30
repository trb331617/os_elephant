#ifndef __USER_SYSCALLINIT_H
#define __USER_SYSCALLINIT_H

void syscall_init(void);

unsigned int sys_getpid(void);
unsigned int sys_write(char *str);

#endif