#ifndef STL_ALLOC_H
#	define STL_ALLOC_H
#	include<iostream>
#	if !defined(__THROW_BAD_ALLOC)
#	include <iostream>
#	define __THROW_BAD_ALLOC cerr <<"out of memory" << endl; exit(1)
#	endif


template <int inst> //inst 不同会得到不同组的静态成员
class __malloc_alloc_template { //第一级分配器，直接调用 malloc,realloc,free
private:
	static void *oom_malloc(size_t); //void * 返回无类型指针
	static void* oom_realloc(void*, size_t);
	static void (*__malloc_alloc_oom_handler)();// 这是一个函数指针

public:

	static void* allocate(size_t n)
	{
		void* result = malloc(n);
		if (0 == result) result = oom_malloc(n);
		return result;
	}

	static void deallocate(void* p, size_t /* n */) {
		free(p);
	}
	static void* reallocate(void* p, size_t /*old_sz*/, size_t new_sz) {
		void* result = realloc(p, new_sz);
		if (0 == result) result = oom_realloc(p, new_sz);
		return result;
	}
	static void (*set_malloc_handler(void(*f)()))()
	{
		void (*old)() = __malloc_alloc_oom_handler;
		__malloc_alloc_oom_handler = f;
		return ( old);
	}
};

template<int inst>
void(*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = 0;

template<int inst>
void *__malloc_alloc_template<inst>::oom_malloc(size_t n) {
	void (*my_malloc_handler)() = 0;
	void* result;
	for (;;) {
		my_malloc_handler = __malloc_alloc_oom_handler;
		if (0 == my_malloc_handler) { __THROW_BAD_ALLOC; }
		(*my_malloc_handler)();
		result = malloc(p, n);
		if (result) return (result);
	}
}
template<int inst>
void* __malloc_alloc_template<inst>::oom_realloc(void* p, size_t n)
{
	void (*my_malloc_handler)() = 0;
	void* result;
	my_malloc_handler = __malloc_alloc_oom_handler;
	if (0 == my_malloc_handler) { __THROW_BAD_ALLOC; }
	(*my_malloc_handler)();
	result = realloc(p, n);
	if (result) return (result);

}

typedef __malloc_alloc_template<0> malloc_alloc;


enum {__ALGIN = 8};
enum {__MAX_BYTES = 128};
enum {__NFREELISTS = 16};

template<bool threads,int inst>
class __default_alloc_template {
private:
	static size_t ROUND_UP(size_t bytes) {
		return (bytes + __ALGIN - 1) & ~(__ALGIN - 1);
	}

	union obj {
		union obj* free_list_link;
		char client_data[1];
	};
	static obj* volatile free_list[__NFREELISTS]; //volatile指针，指向的值是volatile的，需要每次都从内存访问
												  //free_list本质上是16个栈，每个栈对应8*(i+1)大小的chunk
	static size_t FREELIST_INDEX(size_t bytes) {
		return (bytes + __ALIGN - 1) / __ALGIN - 1;
	}
	static void* refill(size_t n);
	static char* chunk_alloc(size_t size, int& nobjs); 
	static char* start_free; //内存池开始位置
	static char* end_free; //内存池结束位置
	static size_t heap_size;
public:
	static void* allocate(size_t n){//大于128bytes就调用一级配置器，小于128bytes就调用ROUND_UP分配对应8倍数的地址空间
		if (n > (size_t)__MAX_BYTES)
			return malloc_alloc::allocate(n);
		obj* volatile *my_free_list;
		obj* result;
		my_free_list = free_list + FREELIST_INDEX(n);
		result = *my_free_list;
		if (0 == result) {
			void* r = refill(ROUND_UP(n));
			return r;
		}
		*my_free_list = result->free_list_link;
		return result;
	} 
	static void deallocate(void* p, size_t n){
		if (n > (size_t)__MAX_BYTES) {
			malloc_alloc::deallocate(p, n);
			return;
		}
		obj* temp = (obj*)p;
		obj* volatile* my_free_list;
		my_free_list = free_list + FREELIST_INDEX(n);
		temp->free_list_link = *my_free_list;
		*my_free_list = temp;
	}
	static void* reallocate(void* p, size_t old_sz) { cerr << "Unimplement of reallocate function !" << endl; exit(1); return NULL; };//没有给出实现，暂时填一个报错函数在这边
};
template<bool threads, int inst>
char* __default_alloc_template<threads, inst>::start_free = 0;
template<bool threads, int inst>
char* __default_alloc_template<threads, inst>::end_free = 0;
template<bool threads, int inst>
size_t __default_alloc_template<threads, inst>::heap_size= 0;

template<bool threads, int inst>
__default_alloc_template<threads, inst>::obj* 
volatile __default_alloc_template<threads,inst>:: free_list[__NFREELISTS] = 
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

template<bool threads, int inst>
static void* __default_alloc_template<threads, inst>::refill(rsize_t n) {//注意此时输入的n已经经过ROUND——UP
	int nobjs = 20;//如果对应大小的chunk没有了，默认填入20新的对应大小的chunk，注意如果内存不足，则可能填入小于20个，这是chunk_alloc int&的原因。
	char* chunk = chunk_alloc(n, nobjs);
	if (nobjs == 1)
		return chunk;

	obj* volatile* my_free_list;
	my_free_list = free_list + FREELIST_INDEX(n);
	obj* result = (obj*)chunk;
	obj* current_obj, * next_obj;
	*my_free_list  = next_obj = (obj*)(chunk + n);
	for (int i = 1;; i++) {
		current_obj = next_obj;
		next_obj = (obj*)((char*)current_obj + n);
		if (i == nobjs - 1) {
			current_obj->free_list_link = 0;
			break;
		}
		else {
			current_obj->free_list_link = next_obj;
		}
	}
	return result;
}

template<bool threads, int inst>
static char* __default_alloc_template<threads, inst>::chunk_alloc(size_t size, int& nobjs) {
	char* result;
	size_t total_bytes = size * nobjs;
	size_t bytes_left = end_free - start_free;
	if (bytes_left >= total_bytes) {//内存池空间满足需求
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	else if (bytes_left >= size) {//有一个以上的大小
		nobjs = bytes_left / size;
		total_bytes = size * nobjs;
		result = start_free;
		start_free += total_bytes;
		return result;
	}
	else {//一个也不剩
		size_t bytes_to_get = 2 * total_bytes + ROUND_UP(heap_size >> 4);
		if (bytes_left > 0) {//把余下的bytes都插入到小块的栈中
			obj* volatile* my_free_list = free_list + FREELIST_INDEX(bytes_left);
			(obj*)start_free->free_list_link = *my_free_list;
			*my_free_list = (obj*)start_free;
		}
		start_free = (char*)malloc(bytes_to_get);
		if (start_free == 0) {//无法找到bytes_to_get大小的chunk,尝试从更大的块中获得内存
			int i;
			obj* volatile* my_free_list, * p;
			for (i = size; i <= __MAX_BYTES, i += __ALGIN) {
				my_free_list = free_list + FREELIST_INDEX(i);
				p = *my_free_list;
				if (p != 0) {
					*my_free_list = p->free_list_link;
					start_free = (char*)p;
					return chunk_alloc(size, nobjs);
				}
			}
			end_free = 0;
			start_free = (char*)malloc_alloc::allocate(bytes_to_get);//山穷水尽，利用OOM机制，要么找到新的chunk，要么报错
		}
		heap_size += bytes_to_get;//找到了这么多
		end_free = start_free + bytes_to_get;
		return chunk_alloc(size, nobjs);
	}
}

typedef __default_alloc_template<0, 0> alloc;//不考虑多线程，threads=0


template<class T, class Alloc>
class simple_alloc {//统一接口
public:
	static T* allocate(size_t n)
	{
		return 0 == n ? 0 : (T*)Alloc::allocate(n * sizeof(T));
	}
	static T* allocate(void)
	{
		return (T*)Alloc:allocate(sizeof(T));
	}
	static void deallocate(T* p, size_t n)
	{
		if (n != 0)
			Alloc::deallocate(p, n * sizeof(T));
	}
	static void deallocate(T* p)
	{
		Alloc::deallocate(p, sizeof(T));
	}
};

#endif // !STL_ALLOC_H
