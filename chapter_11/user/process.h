#ifndef __USER_PROCESS_H
#define __USER_PROCESS_H

#include "thread.h"

// 4GB的虚拟地址空间中，0xc000_0000-1是用户空间的最高地址，0xc000_0000~0xffff_ffff是内核空间
// 由于申请内存时，内存管理模块返回的地址是内存空间的下边界
#define USER_STACK3_VADDR   (0xc0000000 - 0x1000)
#define USER_VADDR_START    0x8048000   // 即128M

#define default_prio        3

void process_activate(struct task_struct *pthread);
void process_execute(void *filename, char *name);

#endif