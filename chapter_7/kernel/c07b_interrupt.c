// FILE: interrupt.c
// TITLE: 构建IDT，初始化8259A

//  根据kernel/core_interrupt.asm中的全局数组intr_entry_table构建中断描述符，再写入IDT

// DATE: 20200307
// USAGE: 

#include "stdint.h"
#include "print.h"

#include "interrupt.h"
#include "global.h"
#include "io.h"

// PIC, Programmable Interrupt Controller
// 这里用的可编程中断控制器是8259A
#define PIC_M_CTRL 0x20	       // 主片的控制端口是0x20
#define PIC_M_DATA 0x21	       // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0	       // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1	       // 从片的数据端口是0xa1

#define IDT_DESC_CNT 0x21	 // 33个，目前总共支持的中断数


// 中断门描述符结构体
// 根据中断描述符定义的结构体，8字节
// 结构体中位置越偏下的成员，其地址越高
struct gate_desc {
   uint16_t    func_offset_low_word;
   uint16_t    selector;
   uint8_t     dcount;   //此项为双字计数字段，是门描述符中的第4字节。此项固定值，不用考虑
   uint8_t     attribute;
   uint16_t    func_offset_high_word;
};

static struct gate_desc idt[IDT_DESC_CNT];   // idt是中断描述符表,本质上就是个中断门描述符数组
extern void* intr_entry_table[IDT_DESC_CNT];	    // 声明引用定义在core_interrupt.asm中的中断处理函数入口数组
void* idt_table[IDT_DESC_CNT];  // 定义中断处理程序数组
                                // 在core_interrupt.asm中定义的intr_xx_entry只是中断处理程序的入口，最终调用的是这里的处理程序

char* intr_name[IDT_DESC_CNT];      // 用于保存异常的名字



 

/* 初始化可编程中断控制器8259A */
static void pic_init(void) 
{

   /* 初始化主片 */
   outb(PIC_M_CTRL, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
   outb(PIC_M_DATA, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
   outb(PIC_M_DATA, 0x04);   // ICW3: IR2接从片. 
   outb(PIC_M_DATA, 0x01);   // ICW4: 8086模式, 正常EOI

   /* 初始化从片 */
   outb(PIC_S_CTRL, 0x11);	// ICW1: 边沿触发,级联8259, 需要ICW4.
   outb(PIC_S_DATA, 0x28);	// ICW2: 起始中断向量号为0x28,也就是IR[8-15] 为 0x28 ~ 0x2F.
   outb(PIC_S_DATA, 0x02);	// ICW3: 设置从片连接到主片的IR2引脚
   outb(PIC_S_DATA, 0x01);	// ICW4: 8086模式, 正常EOI

   /* 打开主片上IR0, 只接受时钟产生的中断, 屏蔽其它外部中断 */
   // 时钟中断将会触发0x20号中断
   outb(PIC_M_DATA, 0xfe);
   outb(PIC_S_DATA, 0xff);

   put_str("   pic_init done\n");
}


/* 创建中断门描述符 */
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, void* function) 
{ 
   p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
   p_gdesc->selector = SELECTOR_K_CODE;
   p_gdesc->dcount = 0;
   p_gdesc->attribute = attr;
   p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}


/*初始化中断描述符表*/
static void idt_desc_init(void) 
{
   int i;
   for (i = 0; i < IDT_DESC_CNT; i++) {
      make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]); 
   }
   put_str("idt_desc_init done\n");
}


/* 通用的中断处理函数 */
static void general_intr_handler(uint8_t intr_id)
{
    // IRQ7和IRQ15会产生伪中断(spurious interrupt)，无需处理
    // 0x2f是从片8259A上的最后一个IRQ引脚，保留项
    if(intr_id == 0x27 || intr_id == 0x2f)
        return;
    
    put_str("interrupt vector: 0x");
    put_int(intr_id);
    put_char('\n');
}


/* 完成一般中断处理函数注册及异常名称注册 */
static void exception_init(void)
{
    int i;
    for(i=0; i<IDT_DESC_CNT; i++)
    {
        // idt_table数组中的函数是进入中断后根据中断向量号调用的
        // 见kernel/core_interrupt.asm的call [idt_table + %1*4]
        idt_table[i] = general_intr_handler;    // 先统一默认为general_intr_handler
        intr_name[i] = "unknown";               // 先统一赋值为unknown
    }
    
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
    
}



/*完成有关中断的所有初始化工作*/
void idt_init() 
{
   put_str("idt_init start\n");
   idt_desc_init();	   // 初始化中断描述符表
   exception_init();   // 异常名称初始化并注册通用的中断处理函数
   pic_init();		   // 初始化8259A

   /* 加载idt */
   // idt地址左移16位，以防原地址高16位不是0而造成数据错误，这里将idt转成64位后再左移
   // 由于指针只能转换成相同大小的整型，所以先将其转换成32位，再64位
   uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)(uint32_t)idt << 16));
   
   // 内联汇编，lidt 把IDT的界限值16位、基地址32位加载到IDTR寄存器
   // lidt的操作数是从内存地址处获取， m 表示内存约束
   asm volatile("lidt %0" : : "m" (idt_operand));
   
   put_str("idt_init done\n");
}