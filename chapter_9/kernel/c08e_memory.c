#include "memory.h"
#include "string.h"
#include "print.h"
#include "debug.h"

#define PAGE_SIZE   4096

/***************  位图地址 ********************
 * 因为0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb.
 * 一个页框大小的位图可表示128M内存, 位图位置安排在地址0xc009a000,
 * 这样本系统最大支持4个页框的位图,即512M
 * 0xc009 e00 - 0x4000 = 0xc009 a00 */
// 一个页框大小的位图，4KB*8bits*4KB=128M
#define MEM_BITMAP_BASE     0xc009a000      // 内存位图基址
/*************************************/

/* 0xc000_0000是内核从虚拟地址3G起.
 * 0x0010_0000意指跨过低端1M内存,使虚拟地址在逻辑上连续 */
#define K_HEAP_START    0xc0100000      // 内核堆空间的起始虚拟地址


/* 二级页表的分页机制下，
 * 高10位为页目录表PDT中的索引，中间10位为页表PT中的索引 */
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)


/* 内存池结构
 * 用于管理内存池中的所有物理内存 */
struct pool{
    struct bitmap pool_bitmap;      // 内存池用到的位图结构，用于管理物理内存
    unsigned int phy_addr_begin;    // 内存池所管理物理内存的起始地址
    unsigned int pool_size;         // 内存池字节容量，本物理内存池的内存容量
};

struct pool kernel_pool, user_pool;     // 内核内存池和用户内存池

struct virtual_addr kernel_vaddr;       // 用于给内核分配虚拟地址


/* 初始化内存池 */
static void mem_pool_init(unsigned int mem_size)
{
    put_str("    mem_pool_init start...\n");
    
/* 页目录PDT大小为1页
 * 第0和第768(0x300)个页目录项pde指向同一个页表PT，即共享1页
 * 第769~1022个页目录项共指向254个页表
 * 最后一个页目录项(第1023个)指向页目录表PDT本身
 * 因此，共256个页，正好1M */
    unsigned int page_table_size = PAGE_SIZE * 256;     // 页目录表和页表占用的字节大小
    
    unsigned int used_mem = page_table_size + 0x100000; // 低端1MB内存
    unsigned int free_mem = mem_size - used_mem;        // 本项目中设置的总内存为32M - 2M
    
    // 1页为4k,不管总内存是不是4k的倍数,
	// 对于以页为单位的内存分配策略，不足1页的内存不用考虑了
    unsigned short int all_free_pages = free_mem / PAGE_SIZE;
    unsigned short int kernel_free_pages = all_free_pages / 2;
    unsigned short int user_free_pages = all_free_pages - kernel_free_pages;
    
    // 位图中的一位表示一页4KB,以字节为单位
    // 为简化位图操作，余数不处理，坏处是这样做会丢内存(1~7页)
    // 好处是不用做内存的越界检查,因为位图表示的内存少于实际物理内存
    unsigned int kbm_length = kernel_free_pages / 8;    // kernel bitmap长度
    unsigned int ubm_length = user_free_pages / 8;      // user bitmap长度
    
    // 物理内存池起始地址
    unsigned int kp_begin = used_mem;   // kernel pool start, 内核物理内存池的起始地址
    unsigned int up_begin = kp_begin + kernel_free_pages * PAGE_SIZE; // user pool start
    
    kernel_pool.phy_addr_begin = kp_begin;
    user_pool.phy_addr_begin = up_begin;
    
    kernel_pool.pool_size = kernel_free_pages * PAGE_SIZE;
    user_pool.pool_size = user_free_pages * PAGE_SIZE;
    
    kernel_pool.pool_bitmap.bitmap_bytes_len = kbm_length;
    user_pool.pool_bitmap.bitmap_bytes_len = ubm_length;
    
    
/*********    内核内存池和用户内存池位图   ***********
 *   位图是全局的数据，长度不固定。
 *   全局或静态的数组需要在编译时知道其长度，
 *   而我们需要在程序运行过程中根据总内存大小算出位图需要多少字节。
 *   所以改为指定一块内存来生成位图，这样就不需要固定长度了
 *   ************************************************/

// 内核使用的最高地址是0xc009f000,这是主线程的栈地址.(内核的大小预计为70K左右)
// 32M内存占用的位图是2k.内核内存池的位图先定在MEM_BITMAP_BASE(0xc009a000)处
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;

// 用户内存池的位图紧跟在内核内存池位图之后
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);
    
    /******************** 输出内存池信息 **********************/
    put_str("    kernel_pool_bitmap_start: 0x"); 
    put_int((int)kernel_pool.pool_bitmap.bits);     // 内存池所用位图的起始地址
    put_str("\n    kernel_pool_phy_addr_start: 0x"); 
    put_int(kernel_pool.phy_addr_begin);    // 内存池的起始物理地址
    
    put_str("\n\n    user_pool_bitmap_start: 0x");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("\n    user_pool_phy_addr_start: 0x");
    put_int(user_pool.phy_addr_begin);
    put_str("\n");
    
/* 将位图置0初始化 
 * 位值为0表示该位对应的内存页未分配，为1表示已分配 */
    bitmap_init(&kernel_pool.pool_bitmap);      // 初始化位图
    bitmap_init(&user_pool.pool_bitmap);
    
    /* 初始化内核虚拟地址池 */
    
    /* 初始化内核虚拟地址的位图，按实际物理内存大小生成数组 */
    // 用于维护内核堆的虚拟地址，所以要和内核内存池大小一致
    kernel_vaddr.vaddr_bitmap.bitmap_bytes_len = kbm_length;
    
    /* 位图的数组指向一块未使用的内存,目前定位在内核内存池和用户内存池之外*/
    // 这里将其安排在紧挨着内核内存池和用户内存池所用的位图之后
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    
    // 内核虚拟内存池的起始地址为K_HEAP_START，即0xc010 0000
    kernel_vaddr.vaddr_begin = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);    // 初始化内核的虚拟内存池位图
    put_str("    mem_pool_init done!\n");
}


/* 内存管理部分初始化入口 */
void mem_init()
{
    put_str("mem_init start...\n");
    
/* loader.asm中把获取到的内存容量保存在汇编变量mem_total_size中, 其物理地址为0x900
 * mem_total_size是用伪指令dd来定义的，宽度为32位。这里先把0x900转换成32位整型指针，
 * 再通过*对该指针做取值操作 */
    unsigned int mem_bytes_total = (*(unsigned int *)(0x900));  // 以字节为单位
    mem_pool_init(mem_bytes_total);     // 初始化内存池
    put_str("mem_init done!\n\n");
}







/* 在用户(pf=2)/内核(pf=1)的虚拟内存池中申请page_count个虚拟页，
 * 成功则返回虚拟页起始地址，失败则返回NULL */
static void *vaddr_get(enum pool_flags pf, unsigned int page_count)
{
    int vaddr_begin = 0;
    unsigned int counting = 0;
    if(pf == PF_KERNEL)     // 内核虚拟内存池
    {
        int bit_index = bitmap_scan(&kernel_vaddr.vaddr_bitmap, page_count);
        if(bit_index  == -1)
            return NULL;
        while(counting < page_count)
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_index + counting++, 1);
        vaddr_begin = kernel_vaddr.vaddr_begin + bit_index * PAGE_SIZE;
    }
    else    // 用户内存池,将来实现用户进程再补充
    {
        
    }
    return (void *)vaddr_begin;
}


/* 得到虚拟地址vaddr对应的页表项PTE指针 */
unsigned int *pte_ptr(unsigned int vaddr)
{
    /* 先访问到页表自己 + 
     * 再用页目录项pde(页目录内页表的索引)做为pte的索引访问到页表 +
     * 再用pte的索引作为页内偏移 */
    unsigned int *pte = (unsigned int *)(0xffc00000 + \
        ((vaddr & 0xffc00000) >> 10) + \
        PTE_IDX(vaddr) * 4);
    return pte;
}


/* 得到虚拟地址vaddr对应的页目录项PDE的指针 */
unsigned int *pde_ptr(unsigned int vaddr)
{
    // 0xffff_f000是用来访问到页目录表本身所在的地址
    unsigned int *pde = (unsigned int *)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}


/* 在mem_pool指向的物理内存池中分配一个物理页，
 * 成功则返回页的物理地址，失败则返回NULL */
static void *palloc(struct pool *mem_pool)
{
    // 扫描或设置位图要保证原子操作
    int bit_index = bitmap_scan(&mem_pool->pool_bitmap, 1); // 找一个物理页
    if(bit_index == -1)
        return NULL;
    bitmap_set(&mem_pool->pool_bitmap, bit_index, 1);
    unsigned int page_phyaddr = bit_index * PAGE_SIZE + mem_pool->phy_addr_begin;
    return (void *)page_phyaddr;
}


/* 页表中添加虚拟地址_vaddr 与物理地址_page_phyaddr的映射 */
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    unsigned int vaddr = (unsigned int)_vaddr, page_phyaddr = (unsigned int)_page_phyaddr;
    
    unsigned int *pde = pde_ptr(vaddr);
    unsigned int *pte = pte_ptr(vaddr);
    
/************************   注意   *************************
 * 执行*pte,会访问到空的pde。所以确保pde创建完成后才能执行*pte,
 * 否则会引发page_fault。因此在*pde为0时,*pte只能出现在下面else语句块中的*pde后面。
 * *********************************************************/
    /* 先在页目录内判断目录项的P位，若为1,则表示该表已存在 */
    if(*pde & 0x00000001) // 页目录项和页表项的第0位为P,此处判断目录项是否存在
    {
        ASSERT(!(*pte & 0x00000001));   // 断言: pte的P位为不存在
        
        if(!(*pte & 0x00000001))    // 创建页表。pte就应该不存在,多判断一下放心
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1); // US=1 RW=1 P=1
        else
        {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else    // 页目录项不存在，所以要先申请一个物理页作为页表并创建页目录项
    {
        // 页表中用到的页一律从内核空间分配
        unsigned int pde_phyaddr = (unsigned int)palloc(&kernel_pool);
        *pde = pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
        
        /* 分配到的物理页地址pde_phyaddr对应的物理内存清0,
         * 避免里面的陈旧数据变成了页表项,从而让页表混乱.
         * 访问到pde对应的物理地址,用pte取高20位便可.
         * 因为pte是基于该pde对应的物理地址内再寻址,
         * 把低12位置0便是该pde对应的物理页的起始 */
        memset((void *)((int)pte & 0xfffff000), 0, PAGE_SIZE);
        
        ASSERT(!(*pte & 0x00000001));   // 断言: pte的P位为不存在
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}


/* 分配page_count个页空间，成功则返回起始虚拟地址，失败则返回NULL
 * 虚拟地址是连续的，但物理地址可能连续，也可能不连续
 * 一次性申请page_count个虚拟页，成功申请之后，根据申请的页数，通过循环
 * 依次为每一个虚拟页申请物理页，再将它们在页表中依次映射关联 */

void *malloc_page(enum pool_flags pf, unsigned int page_count)
{
    // 本项目中设置的总内存为32M(bochsrc.cfg)，内核和用户空间各约16MB，
    // 保守起见用15MB来限制。15M/4KB=3840页
    ASSERT(page_count > 0 && page_count < 3840);
    
/***********   malloc_page的原理是三个动作的合成:   ***********
      1 通过vaddr_get在虚拟内存池中申请虚拟地址
      2 通过palloc在物理内存池中申请物理页
      3 通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
***************************************************************/ 

    void *vaddr_begin = vaddr_get(pf, page_count);
    if(vaddr_begin == NULL)
        return NULL;
    
    unsigned int vaddr = (unsigned int)vaddr_begin, count = page_count;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    
    /* 虚拟地址是连续的，但物理地址可以是不连续的，所以逐个做映射 */
    while(count-- > 0)
    {
        void *page_phyaddr = palloc(mem_pool);
        if(page_phyaddr == NULL)
        {
            // 物理页申请失败时，应将曾经已申请的虚拟地址和物理页全部回滚，
            // 如果物理内存池的位图也被修改过，还要再把物理地址回滚
            // 在将来完成内存回收时再补充
            return NULL;
        }
        page_table_add((void *)vaddr, page_phyaddr); // 在页表中做映射
        vaddr += PAGE_SIZE;     // 下一个虚拟页
    }
    return vaddr_begin;
}


/* 从内核物理内存池中申请page_count页内存，
 * 成功则返回其虚拟地址，失败则返回NULL */
void *get_kernel_pages(unsigned int page_count)
{
    void *vaddr = malloc_page(PF_KERNEL, page_count);
    if(vaddr != NULL)
        // 若分配的地址不为空，将页清0后返回
        memset(vaddr, 0, page_count * PAGE_SIZE);
    return vaddr;
}
