#include "thread.h"
#include "string.h"
#include "memory.h"

#include "interrupt.h"

#include "print.h"
#include "debug.h"

#define PAGE_SIZE 4096

// 定义在thread/switch.asm中
extern void switch_to(struct task_struct* current, struct task_struct* next);

struct task_struct *main_thread;    // 主线程PCB

struct list thread_ready_list;      // 就绪队列

// 当线程因为某些原因阻塞了，不能放在就绪队列中
struct list thread_all_list;        // 所有任务队列

static struct list_elem *thread_tag;    // 用于保存队列中的线程结点


/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func *function, void *func_arg)
{
    // 执行function前要开中断，避免后面的时钟中断被屏蔽，而无法调度其它线程
    intr_enable();
    
    function(func_arg);
}


/* 初始化线程栈thread_stack
 * 将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg)
{
    /* 先预留中断使用栈的空间，thread.h中定义的结构体 */
    pthread->self_kstack -= sizeof(struct intr_stack);  // self_kstack在init_thread()中已初始化为PCB最顶端
    
    /* 再预留线程栈空间，thread.h中定义的结构体 */
    pthread->self_kstack -= sizeof(struct thread_stack);
    
    // 初始化线程栈thread_stack
    struct thread_stack *kthread_stack = (struct thread_stack *)pthread->self_kstack;
    
    // ip 指令指针寄存器
    kthread_stack->eip = kernel_thread;     // 函数kernel_thread
        // 结合线程栈struct thread_stack的定义，当处理器进入kernel_thread函数体时，
        // 栈顶为返回地址、栈顶+4为参数function、栈顶+8为参数func_arg
    
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    // 将这4个寄存器初始化为0，因为线程中的函数尚未执行，执行之后寄存器才会有值
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}


/* 初始化线程基本信息 */
void init_thread(struct task_struct *pthread, char *name, int priority)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    
    if(pthread == main_thread)
        // 由于把main函数也封装成一个线程，并且它一直是运行的，故设为TASK_RUNNING
        pthread->status = TASK_RUNNING;
    else
        pthread->status = TASK_READY;
    
    // self_kstack是线程自己在内核态下使用的栈顶地址
    pthread->self_kstack = (unsigned int *)((unsigned int)pthread + PAGE_SIZE); // 参数phtread为最低地址
    pthread->priority = priority;
    pthread->ticks = priority;
    pthread->elapsed_ticks = 0;
    pthread->pgdir = NULL;
    
    pthread->stack_magic = 0x19870916;      // 自定义的魔数
}


/* 创建优先级为priority的线程，线程名为name，线程所执行的函数是function(func_arg) */
struct task_struct *thread_start(char *name, int priority, thread_func function, void *func_arg)
{
    // PCB都位于内核空间，包括用户进程的PCB也是在内核空间
    struct task_struct *thread = get_kernel_pages(1);    // 在内核空间中申请一页内存
                                                        // thread指向的是PCB的最低地址
    
    init_thread(thread, name, priority);        // 初始化线程基本信息
    thread_create(thread, function, func_arg);  // 初始化线程栈
    
    
    // 确保之前不在队列中
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    // 加入就绪线程队列
    list_append(&thread_ready_list, &thread->general_tag);
    
    // 确保之前不在队列中
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    // 加入全部线程队列
    list_append(&thread_all_list, &thread->all_list_tag);
    
    
    /* 执行完这句汇编后，线程就会开始执行 */
    // 输入部分，通用约束"g"表示内存或寄存器都可以，栈顶self_kstack值赋给esp
    // thread_create()中初始化的0弹入到4个相应的寄存器中
    // ret 把栈顶的数据作为返回地址送上处理器的EIP寄存器，即开始执行 kernel_thread()函数
    // asm volatile("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g"(thread->self_kstack) : "memory");
    
    return thread;
}





/* 获取当前线程PCB指针 */
struct task_struct *running_thread()
{
    unsigned int esp;
    asm ("mov %%esp, %0" : "=g"(esp));
    
    // 取esp整数部分，即PCB起始地址
    return (struct task_struct *)(esp & 0xfffff000);
}


/* 将kernel中main函数完善为主线程 */
static void make_main_thread(void)
{
    // 因为main线程早已运行
    // 在loader.asm中进入内核时mov esp, 0xc009_f000就是为其预留的PCB
    // 因此pcb地址为0xc009_e000，不需要通过get_kernel_page另分配一页
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    
    // main函数是当前线程，当前线程不在thread_ready_list中，所以只加在thread_all_list中
    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}




/* 实现任务调度
 * 将当前线程换下处理器，并在就绪队列中找出下个可运行的程序，将其换上处理器 */
// 由device/timer.c中的时钟中断处理函数调用
void schedule()
{
    // 处理器进入中断后，会自动把标志寄存器eflags中的IF位置0，即中断处理程序在关中断的情况下运行
    // 能进入中断处理程序就表示已经处在关中断情况下
    ASSERT(intr_get_status() == INTR_OFF);
    
    struct task_struct *current_thread = running_thread();
    if(current_thread->status == TASK_RUNNING)
    {
        // 若此线程只是cpu时间片到了，将其加入到就绪队列尾
        ASSERT(!elem_find(&thread_ready_list, &current_thread->general_tag));
        list_append(&thread_ready_list, &current_thread->general_tag);
        
        current_thread->ticks = current_thread->priority; // 将此线程的ticks重置为其priority
        current_thread->status = TASK_READY;
    }
    else
    {
        // 若此线程需要某事件发生后才能继续上CPU运行，不需要将其加入队列，
        // 因为当前线程不在就绪队列中
    }
    
    // 尚未实现idle线程，因此有可能就绪队列为空
    ASSERT(!list_empty(&thread_ready_list));
    thread_tag = NULL;  // thread_tag清空
    
    // 将thread_ready_list队列中的第一个就绪线程弹出，准备将其调度上CPU
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct *next = elem2entry(struct task_struct, general_tag, thread_tag);
    next->status = TASK_RUNNING;
    
    // 将线程current的上下文保护好，再将线程next的上下文转载到处理器，从而完成任务切换
    switch_to(current_thread, next);    // 准备切换寄存器映像
}



/* 初始化线程环境 */
void thread_init(void)
{
    put_str("thread_init start...\n");
    
    list_init(&thread_ready_list);  // 初始化就绪队列
    list_init(&thread_all_list);    // 初始化全部队列
    
    // 将当前main函数创建为线程
    make_main_thread();
    
    put_str("thread_init done\n\n");
}