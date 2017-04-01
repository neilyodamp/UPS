#ifndef INCLUDED_tlsf
#define INCLUDED_tlsf
#include <stddef.h>

#if defined(__cplusplus)
extern "C"{
#endif

typedef void* tlsf_pool;

//创建和删除内存池
tlsf_pool tlsf_create(void* mem,size_t bytes);
void tlsf_destroy(tlsf_pool pool);

//分配/内存对齐/重分配/释放 替换
void* tlsf_malloc(tlsf_pool pool,size_t bytes);
void* tlsf_memalign(tlsf_pool pool,size_t align,size_t bytes);
void* tlsf_realloc(tlsf_pool pool,void* ptr,size_t size);
void tlsf_free(tlsf_pool pool,void* ptr);

/* Debugging */
typedef void (*tlsf_walker)(void* ptr,size_t size,int used,void* user);
void tlsf_walk_heap(tlsf_pool pool,tlsf_walker walker,void* user);
//如果堆检查失败,返回非零值
int tlsf_check_heap(tlsf_pool pool);

// 返回内部block大小 而不是源请求分配大小
size_t tlsf_block_size(void* ptr);


size_t tlsf_overhead();


#if defined(__cplusplus)
};
#endif

#endif