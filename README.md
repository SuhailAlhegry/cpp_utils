# ACHILLES Utils

A bunch of utilities I use when writing C++.

## Assertion 

In [assert.hpp](./assert.hpp). A very simple assertion function prototype, users need to to define the actual function. this design allows the user to define a tracing assertion function for example, where it prints better assertion details for each platform. The program exists with the return code of `1` when the condition is not met.

usage:
```c++
#include <cstdio>
#include <utils/assert.hpp>

// a simple implementation of 'aassert_handler' that just prints the message
// not the double 'aa' in 'aassert'
inline bool aassert_handler(const char *file, int line, const char *conditionCode, const char *message) {
    printf("(%s, %i) assertion '%s' failed: %s", file, line, conditionCode, message);
    return true; // remember to always return true, a quirk I might remove later
}

void main() {
    // again the double 'aa'
    aassert(false, "my message"); // this is triggered
}
```

## Defer

In [defer.hpp](./defer.hpp). A very simple, but very helpful construct, inspired by go's `defer`, and D's `scope_exit`, the implementation is courtesy of Andrei Alexandrescu. It uses a temporary object's destructor and a lambda to achieve the functionality.

usage:
```c++
#include <cstdio>
#include <utils/defer.hpp>

void main() {
    printf("hello at start");
    defer { printf("hello at end"); }; // executes the code after the scope exists
    printf("hello at middle");
}
```

## Enums

In [enums.hpp](./enums.hpp). A monstrosity, totally-not-understandable-at-first-glance-way to generate scoped enums with pretty printing. The usage however, is pretty simple, and supports auto-completion if you need that!

```c++
#include <cstdio>
#include <utils/enums.hpp>
#include <utils/types.hpp> // for 'u32'

// define an enum
ENUM(MyEnum, First, Second, Third);
// define an enum with a type
ENUM_T(MyTypedEnum, u32, First, Second, Third);

void main() {
    MyEnum e = MyEnum::First; // note the scope
    printf("my enum value is: %s", e.toString()); // prints "my enum value is: First"
}
```

## Memory
In [memory.hpp](./memory.hpp). Simple structures that use and manipulate memory, this file also includes the `Allocator` interface, which is a simple but powerful interface. The implementation of the allocator is left to the user, but it is trivial to create such allocators, since the interface allows for composable allocators. Design also inspired by the work of Andrei Alexandrescu.

```c++
#include <utils/memory.hpp>

using achilles::memory;

// a simple allocator that uses malloc, zeroes out memory on allocation and resize
struct MallocAllocator : Allocator {
    Block allocate(u64 size) override {
        u8 *mem = (u8 *) malloc(size);
        if (mem == nullptr) {
            return Block {
                nullptr,
                0,
            };
        } else {
            memset(mem, 0, size);
            return Block {
                mem,
                size,
            };
        }
    }

    bool tryResize(Block &block, u64 newSize) override {
        void *ptr = realloc(block.memory, newSize);
        if (ptr == nullptr) return false;
        block.memory = (u8 *) ptr;
        if (newSize > block.size) {
            memset(block.memory + block.size, 0, newSize - block.size);
        }
        block.size = newSize;
        return true;
    }

    void deallocate(Block &block) override {
        free(block.memory);
        block.memory = nullptr;
        block.size = 0;
    }
};

// a simple allocator that allocates on the stack
template<u64 SIZE>
struct StackAllocator : Allocator {
    Block allocate(u64 size) override {
        if (!canAllocate(size)) {
            return Block {
                nullptr,
                0,
            };
        }

        u8 *mem = _ptr;
        _ptr += size;
        return Block {
            mem,
            size,
        };
    }

    bool tryResize(Block &block, u64 newSize) override {
        u64 current = (u64) _ptr;
        u8 *blockBase = (u8 *)(current - block.size);
        if (blockBase != block.memory) return false;
        s64 diff = ((s64) newSize) - ((s64) block.size);
        s64 base = (s64) _memory;
        s64 max = base + SIZE;

        if (current + diff <= max) {
            _ptr += diff;
            if (newSize > block.size) {
                memset(block.memory + block.size, 0, newSize - block.size);
            }
            block.size = newSize;
            return true;
        }

        return false;
    }

    void deallocate(Block &block) override {
        if (!owns(block) || !canDeallocate(block)) return;
        _ptr -= block.size;
        block.memory = nullptr;
        block.size = 0;
    }

    bool owns(Block const &block) const override {
        if (block.memory == nullptr) return false;
        u64 address = (u64) _memory;
        u64 mem = (u64) block.memory;
        return mem >= address && mem <= address + SIZE;
    }

    bool canAllocate(u64 size) const override {
        s64 base = (s64) _memory;
        s64 max = base + SIZE;
        s64 ptr = (s64) _ptr;
        return (ptr + size) <= max;
    }

    bool canDeallocate(Block const &block) const override {
        if (block.memory == nullptr) return false;
        u64 current = (u64) _ptr;
        u8 *diff = (u8 *) (current - block.size);
        return diff == block.memory;
    }
private:
    u8 _memory[SIZE] {0};
    u8 *_ptr = _memory;
};

// a fallback allocator, tries 'Primary' first, if it fails defers to 'Secondary'
template<typename Primary, typename Secondary>
struct FallbackAllocator : Allocator {
    Block allocate(u64 size) override {
        Block blk = primary.allocate(size);
        if (blk.isValid()) {
            return blk;
        }
        return secondary.allocate(size);
    }

    bool tryResize(Block &block, u64 newSize) override {
        if (primary.owns(block)) {
            if (primary.tryResize(block, newSize)) return true;
            if (primary.canDeallocate(block)) {
                Block newBlk = secondary.allocate(newSize);
                if (newBlk.isValid()) {
                    memcpy(newBlk.memory, block.memory, block.size);
                    secondary.deallocate(block);
                    block.memory = newBlk.memory;
                    block.size = newBlk.size;
                    newBlk.memory = nullptr;
                    newBlk.size = 0;
                    return true;
                }
            }

            return false;
        } else {
            return secondary.tryResize(block, newSize);
        }
    }

    void deallocate(Block &block) override {
        if (primary.owns(block)) {
            primary.deallocate(block);
        } else {
            secondary.deallocate(block);
        }
    }

    bool owns(Block const &block) const override {
        return primary.owns(block) || secondary.owns(block);
    }

    bool canAllocate(u64 size) const override { 
        return primary.canAllocate(size) || secondary.canAllocate(size);
    }

    bool canDeallocate(Block const &block) const override {
        if (!owns(block)) return false;
        if (primary.owns(block)) {
            return primary.canDeallocate(block);
        }
        return secondary.canDeallocate(block);
    }
private:
    Primary primary;
    Secondary secondary;
};

// a simple stats tracker, can be composed with any allocator, it tracks the allocations and resize calls of the allocator
template<typename Allocator>
struct SimpleAllocationTracker : Allocator {
    Block allocate(u64 size) override {
        _allocations += 1;
        return Allocator::allocate(size);
    }

    bool tryResize(Block &block, u64 newSize) override {
        _resizes += 1;
        bool result = Allocator::tryResize(block, newSize);
        if (result) {
            _successfulResizes += 1;
        } else {
            _failedResizes += 1;
        }
        return result;
    }

    void deallocate(Block &block) override {
        _deallocations += 1;
        Allocator::deallocate(block);
    }

    bool owns(Block const &block) const override {
        return Allocator::owns(block);
    }

    bool canAllocate(u64 size) const override { 
        return Allocator::canAllocate(size);
    }

    bool canDeallocate(Block const &block) const override {
        return Allocator::canDeallocate(block);
    }

    s64 leaks() const {
        return _allocations - _deallocations;
    }

    s64 allocations() const {
        return _allocations;
    }

    s64 deallocations() const {
        return _deallocations;
    }

    s64 resizes() const {
        return _resizes;
    }

    s64 successfulResizes() const {
        return _successfulResizes;
    }

    s64 failedResizes() const {
        return _failedResizes;
    }

private:
    s64 _allocations;
    s64 _deallocations;
    s64 _resizes;
    s64 _successfulResizes;
    s64 _failedResizes;
};

// a replacement for operator 'new' that uses the 'Allocator' interface
template<typename T, typename...Args>
Address<T> make(Allocator *allocator, Args... args) {
    Address<T> addr = allocator->allocate(sizeof(T));
    new (addr) T {std::forward<Args>(args)...};
    return addr;
}

int main() {
    // allocate 1KB on the stack and use it as an allocator
    auto stacklocator = StackAllocator<1024>{};

    // or..just use malloc
    auto mallocator = MallocAllocator{};

    // or better..use a fallback allocator
    auto fallocator = FallbackAllocator<StackAllocator<1024>, MallocAllocator>{};

    // or even better..wrap your allocator with a stats tracker
    auto allocator = SimpleAllocationTracker<MallocAllocator>{};
    defer { printf("allocator leaked: %lls times!\n", allocator.leaks()); };

    // allocate a block of size 10 bytes
    Block b = allocator.allocate(10);
    // remember to deallocate blocks, even though it is not necessary in the case of 'stacklocator'
    defer { allocator.deallocate(b); }

    // allocate an address
    // an address is wrapper around Block that allows the block to behave like a pointer to an object
    Address<u64> a = allocator.allocate(sizeof(u64));
    // deallocate an address directly
    allocator.deallocate(a);

    // a better approach is to use something like 'make'
    auto a1 = make<u64>(&allocator);
    // defer the deallocation
    defer { allocator.deallocate(a1); };
}

```

## Types

In [types.hpp](./types.hpp). Defines the basic numeric types, from `s8` to `s64` for signed integers, and `u8` to `u64` for unsigned integers, `f32` for single precision floats and `f64` for double precision floats, along with the minima and maxima of every integer type. Also defines `typehash` which allows for generating a compile-time hash for any type, and `Any` which is a structure mainly used when wanting to accept any argument for a function, since it doesn't allocate anything and just creates a pointer to its bound value.

```c++
#include <cstdio>
#include <utils/types.hpp>

void anyTest(Any any) {
    switch (any.type()) {
        case typehash<int>: {
            printf("it's an int: %i\n", any.value<int>());
        } break;
        case typehash<unsigned int>: {
            printf("it's an unsigned int: %u\n", any.value<unsigned int>());
        } break;
        case typehash<float>: {
            printf("it's a float: %f\n", any.value<float>());
        } break;
        case typehash<char *>: {
            printf("it's a c string: %s\n", any.value<char *>());
        } break;
        default: {
            printf("type not supported.\n");
        } break;
    }
}

int main() {
    anyTest(123);
    u32 u = 0x123; // u32 is an alias for unsigned int, so it works
    anyTest(u);
    anyTest(456.0f);
    anyTest("hello from any");
    anyTest(3.14); // won't work, since 'anyTest' doesn't supports doubles
}
```

## Files
In [files.hpp](./files.hpp). This is the only utility that probably shouldn't be here, since it depends on the platform, but I use it in almost every project when I need to read a simple file.

usage:
```c++
#include <cstdio>
#include <utils/memory.hpp>
#include <utils/files.hpp>
#include <utils/defer.hpp>
#include <utils/types.hpp>
#include "my_allocators.hpp"

int main() {
    // prepare some memory, ON THE STACK!
    auto allocator = StackAllocator<KB(10)>{};

    // did you notice..we just read a file into our stack!
    Block memory = readFile("main.cpp", allocator);
    defer { allocator.deallocat(memory); };

    // ... do something with the memory

    // write the edited file to disk
    writeToFile("main.cpp", memory);
}
```

## Math
In [math.hpp](./math.hpp). A very basic math library.
