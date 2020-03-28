#include "init.h"
#include "print.h"
#include "interrupt.h"

#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"

/* 初始化所有模块 */
void init_all()
{
    put_str("init_all...\n\n");
    
    idt_init();     // 初始化中断

    mem_init();     // 初始化内存管理系统

    thread_init();  // 初始化线程相关结构
    
    timer_init();   // 初始化PIT, 可编程定时计时器Programmable Interval Timer
    
    console_init(); // 初始化控制台
    
    keyboard_init(); // 初始化键盘
    
    tss_init();     // 初始化TSS(任务状态段)    
}