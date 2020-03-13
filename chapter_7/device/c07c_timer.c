/* 
 * FILE: c07c_timer.c
 * TITLE: 使用8253来提高时钟中断的频率

 * 使用8253来给IRQ0引脚上的时钟中断信号“提速”，使其发出的中断信号频率快一些。它默认的频率是18.206Hz，即一秒内大约发出18 次中断信号。
 * 通过对8253 编程，使时钟一秒内发100 次中断信号，即中断信号频率为100Hz.

 * DATE: 20200313
 * USAGE:
 *
 */

#include "timer.h"
#include "io.h"
#include "print.h"

#define IRQ0_FREQUENCY      100     // 时钟中断的频率，这里我们设置为100Hz    
#define INPUT_FREQUENCY     1193180 // 计数器0的工作脉冲信号频率
#define TIMER0_VALUE        INPUT_FREQUENCY / IRQ0_FREQUENCY    // 计数器的计数初值
#define TIMER0_PORT         0x40    // 端口号，用来指定初始值value的目的端口号
#define TIMER0_ID           0       // 控制字中选择计数器的号码    
#define TIMER_MODE          2       // 计数器的工作方式，2为比率发生器
#define READ_WRITE_LATCH    3       // 计数器的读/写/锁存方式

// PIT, 可编程定时计时器Programmable Interval Timer
#define PIT_CONTROL_PORT    0x43

/* 把操作的计数器id 读写属性rwl 计数器模式mode 写入模式控制寄存器，并赋初始值value */
static void frequency_set(uint8_t counter_port, \
                uint8_t counter_id, \
                uint8_t rwl, \
                uint8_t counter_mode, \
                uint16_t counter_value)
{
    // 往控制寄存器端口0x43写入控制字
    outb(PIT_CONTROL_PORT, (uint8_t)(counter_id << 6 | rwl << 4 | counter_mode << 1));
    // 先写入 counter_value低8位
    outb(counter_port, (uint8_t)counter_value);
    // 再写入 counter_value高8位
    outb(counter_port, (uint8_t)counter_value >> 8);
}


/* 初始化PIT 8253 */
void timer_init()
{
    put_str("timer_init begin …\n");
    // 设置8253定时周期，也就是发中断的周期
    frequency_set(TIMER0_PORT, TIMER0_ID, READ_WRITE_LATCH, TIMER_MODE, TIMER0_VALUE);
    put_str("timer_init success.\n");
}