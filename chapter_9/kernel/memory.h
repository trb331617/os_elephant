#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "bitmap.h"

/* 内存池标记,用于判断用哪个内存池 */
enum pool_flags {
   PF_KERNEL = 1,    // 内核内存池
   PF_USER = 2	     // 用户内存池
};


#define	 PG_P_1	  1	// 页表项或页目录项存在属性位，1存在，0不存在
#define	 PG_P_0	  0	
#define	 PG_RW_R  0	// R/W 属性位值, 读/执行
#define	 PG_RW_W  2	// R/W 属性位值, 读/写/执行
#define	 PG_US_S  0	// U/S 属性位值, 系统级，只允许特权级0 1 2程序访问此页
#define	 PG_US_U  4	// U/S 属性位值, 用户级，允许所有特权级程序访问此页



/* 虚拟地址池，用于管理虚拟地址 */
// 相比于struct pool，这里没有pool_size。虚拟地址空间4GB，
// 相对来说是无限的，不需要指定地址空间大小
struct virtual_addr{
    struct bitmap vaddr_bitmap;     // 虚拟地址用到位图结构
    unsigned int vaddr_begin;       // 虚拟地址起始值
};

// extern struct pool kernel_pool, user_pool;

void mem_init(void);


void *get_kernel_pages(unsigned int page_count);
void *malloc_page(enum pool_flags pf, unsigned int page_count);

unsigned int *pte_ptr(unsigned int vaddr);
unsigned int *pde_ptr(unsigned int vaddr);


#endif