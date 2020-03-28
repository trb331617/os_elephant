; FILE: core_interrupt.asm
; TITLE: 定义了33个中断处理程序，并将中断处理程序的地址保存至全局数组table_intr_entry
;   全局数组table_intr_entry将提供给kernel/interrupt.c用于构建IDT

; 每个中断处理程序都一样，就是调用字符串打印函数put_str
; 中断向量0~19是处理器内部固定的异常类型, 20~31是Intel保留
; 所以可用的中断向量号最低是32，我们在设置8259A时会把IR0的中断向量号设置为32

; DATE: 20200307
; USAGE: 


; 这里是用宏VECTOR实现的中断入口，所有的中断入口程序几乎都一样，只是中断向量号不同
; 宏展开后，中断入口程序名为intr_%1_entry。其中，%1是中断向量号
; 入口程序用这个中断向量号作为idt_table中的索引，调用最终C语言版的中断处理程序



[bits 32]

; 有的中断会产生错误码，用来指明中断是在哪个段上发生的
; 4字节的错误码会在进入中断后，处理器在栈中压入eip之后压入
; 为了保持代码模板的通用性，这里分别做了处理
%define ERROR_CODE nop ; no operation, 不操作，什么都不干
    ; 若在相关的异常中cpu已经自动压入了错误码,为保持栈中格式统一,这里不做操作.
%define ZERO push 0		 ; 若在相关的异常中cpu没有压入错误码,为了统一栈中格式,就手工压入一个0

extern idt_table    ; idt_table是interrupt.c中注册的中断处理函数数组

SECTION .data

global intr_entry_table     ; 全局数组
intr_entry_table:

%macro VECTOR 2
; 本文件中调用该宏33次，在预处理之后，宏中的2个section将会各出现33次
SECTION .text
intr_%1_entry:		 ; 每个中断处理程序都要压入中断向量号,所以一个中断类型一个中断处理程序，自己知道自己的中断向量号是多少
   %2       ; 这里会展开为nop或push 0
   
   ; 在汇编文件中调用C程序一定会破坏当前寄存器环境，所以要先保护现场
   push ds
   push es
   push fs
   push gs
   pushad   ; push all double word register
            ; 依次压入eax ecx edx ebx esp ebp esi edi
      
    ; 由于我们在设置8259A时会设置为手动结束，所以中断处理程序中需手动向8259A发送中断结束标志
   ; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI 
   mov al, 0x20                   ; 中断结束命令EOI, End Of Interrupt
   out 0xa0, al                   ; 向从片发送
   out 0x20, al                   ; 向主片发送
   
   push %1 ; 不管idt_table中的处理函数是否需要参数，这里都一律压入中断向量号

    call [idt_table + %1*4]     ; 调用idt_table中的C语言中断处理函数
    jmp intr_exit

; 编译器会将属性相同的SECTION合并到同一个大的segment中
; 这里的.data将会和上面的紧凑地排在一起
SECTION .data
   dd    intr_%1_entry	 ; 存储各个中断入口程序的地址，形成intr_entry_table数组
%endmacro

SECTION .text
global intr_exit
intr_exit:
    add esp, 4  ; 跳过栈中压入的中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4  ; 跳过栈中error_code
    iretd

; 
VECTOR 0x00,ZERO
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO    ; 时钟中断对应的入口
VECTOR 0x21, ZERO   ; 键盘中断对应的入口
VECTOR 0x22, ZERO   ; 级联用的
VECTOR 0x23, ZERO   ; 串口2对应的入口
VECTOR 0x24, ZERO   ; 串口1对应的入口
VECTOR 0x25, ZERO   ; 并口2对应的入口
VECTOR 0x26, ZERO   ; 软盘对应的入口
VECTOR 0x27, ZERO   ; 并口1对应的入口
VECTOR 0x28, ZERO   ; 实时时钟对应的入口
VECTOR 0x29, ZERO   ; 重定向
VECTOR 0x2a, ZERO   ; 保留
VECTOR 0x2b, ZERO   ; 保留
VECTOR 0x2c, ZERO   ; ps/2鼠标
VECTOR 0x2d, ZERO   ; fpu浮点单元异常
VECTOR 0x2e, ZERO   ; 硬盘
VECTOR 0x2f, ZERO   ; 保留