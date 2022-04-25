#ifndef RYUK_MEMORY_H
#define RYUK_MEMORY_H

#include <corecrt_malloc.h>
#include <string>
#include <vcruntime_string.h>
#include "ryuk_misc.hpp"
#include "ryuk_types.hpp"

template<typename T, u64 defaultAlignment = sizeof(s64)>
class Allocator;

template<typename T>
struct MemoryAddress {
    T *ptr;
    u64 size;

    T& operator[] (u64 index) {
        rassert(ptr != nullptr && size > 0, "address is invalid");
        rassert(between(index, 0, size - 1), "address access out of bounds");
        return ptr[index];
    }
};

template<typename T>
using fn_allocation = MemoryAddress<T> (*)(u64 size, u64 alignment);
template<typename T>
using fn_reallocation = void (*)(MemoryAddress<T> &address, u64 size, u64 alignment);
template<typename T>
using fn_deallocation = void (*)(MemoryAddress<T> &address);

template<typename T, u64 defaultAlignment>
class Allocator {
private:
    fn_allocation<T> _alloc;
    fn_reallocation<T> _realloc;
    fn_deallocation<T> _dealloc;

public:
    Allocator(
        fn_allocation<T> alloc,
        fn_reallocation<T> realloc,
        fn_deallocation<T> dealloc
    ) : _alloc(alloc), _realloc(realloc), _dealloc(dealloc) {}
    
    MemoryAddress<T> alloc(u64 size, u64 alignment = defaultAlignment) {
        return _alloc(size, alignment);
    }
    
    void realloc(MemoryAddress<T> &address, u64 size, u64 alignment = defaultAlignment) {
        _realloc(address, size, alignment);
    }
    
    void dealloc(MemoryAddress<T> &address) {
        _dealloc(address);
    }
};

inline u64 align(u64 size, u64 alignment) {
    return (size + (alignment - 1)) & ~(alignment -1);
}

template<typename T>
MemoryAddress<T> default_alloc(u64 size, u64 alignment = sizeof(s64)) {
    if (size == 0) {
        size = 1;
    }
    T *ptr = (T *) malloc(align(size * sizeof(T), alignment));
    memset(ptr, 0, size);
    return MemoryAddress<T> {
        ptr,
        size
    };
}

template<typename T>
void default_realloc(MemoryAddress<T> &address, u64 size, u64 alignment = sizeof(s64)) {
    rassert(address.ptr != nullptr && address.size != 0, "trying to reallocate freed address");
    address.ptr = (T *) realloc(address.ptr, align(size * sizeof(T), alignment));
    if (size > address.size) {
        memset(address.ptr + address.size, 0, size - address.size);
    }
    address.size = size;
}

template<typename T>
void default_dealloc(MemoryAddress<T> &address) {
    rassert(address.ptr != nullptr && address.size != 0, "trying to free an already freed address");
    free(address.ptr);
    address.ptr = nullptr;
    address.size = 0;
}

template<typename T, u64 defaultAlignment = sizeof(s64)>
struct DefaultAllocator : Allocator<T, defaultAlignment> {
    DefaultAllocator() : Allocator<T, defaultAlignment>(&default_alloc<T>, &default_realloc<T>, &default_dealloc<T>) {}
};

#endif
