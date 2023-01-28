#if !defined(ACHILLES_MEMORY_HPP)
#define ACHILLES_MEMORY_HPP

// this file depends on <cstring> for 'std::memcpy' and friends
// should be able to remove this dependency when providing own 'std::memcpy' implementation
#include <cstring>
// require cstdlib for malloc, free
#include <cstdlib>
#include <initializer_list>
#include "types.hpp"
#include "assert.hpp"

#define KB(s) ((s) * 1024ULL)
#define MB(s) ((s) * KB(1024ULL))
#define GB(s) ((s) * MB(1024ULL))

namespace achilles {
    namespace memory {
        struct Block;

        struct Allocator {
            virtual Block allocate(u64 size) = 0;
            virtual bool tryResize(Block &block, u64 newSize) = 0;
            virtual void deallocate(Block &block) = 0;
            virtual bool owns(Block const &) const { return true; }
            virtual bool canAllocate(u64) const { return true; }
            virtual bool canDeallocate(Block const &) const { return true; }
       };

        struct Block {
            Block(u8 *memory, u64 size) : memory{memory}, size{size} {}
            Block(Block &&other) {
                memory = other.memory;
                size = other.size;
                other.memory = nullptr;
                other.size = 0;
            }

            ~Block() {
                aassert(memory == nullptr, "memory leak");
            }

            template<typename T>
            operator T*() const {
                return (T *) memory;
            }

            bool isValid() const {
                return memory != nullptr && size > 0;
            }

            u8 *memory;
            u64 size;
        };

        template<typename T>
        struct Array {
            using type = T;

            Array(Allocator &allocator, u64 capacity = 8) : _allocator{allocator} {
                Block blk = allocator.allocate(capacity * sizeof(type));
                _block.memory = blk.memory;
                _block.size = blk.size;
                blk.memory = nullptr;
                blk.size = 0;
            }

            u64 size() const {
                return _size;
            }

            u64 capacity() const {
                return _block.size / sizeof(type);
            }

            bool isValid() const {
                return _block.isValid();
            }

            bool push(type value) {
                if (!isValid()) return false;
                if (capacity() == _size) {
                    if (!_allocator.tryResize(_block, _block.size * 2)) {
                        return false;
                    }
                }
                type *memory = _block;
                memory[_size++] = value;
                return true;
            }

            type &pop() {
                aassert(isValid(), "popping from an invalid array");
                aassert(_size > 0, "popping from an empty array");
                type *memory = _block;
                return memory[--_size];
            }

            type &get(u64 index) const {
                aassert(isValid(), "getting a value from an invalid array");
                aassert(_size > 0, "getting a value from an empty array");
                type *memory = _block;
                return memory[index];
            }

            void clear() {
                _size = 0;
            }

            void destroy() {
                _allocator.deallocate(_block);
                clear();
            }

            Block &operator &() {
                return _block;
            }
        private:
            Allocator &_allocator;
            Block _block { nullptr, 0 };
            u64 _size = 0;
        };

        template<typename T, typename W>
        struct RelativePointer {
            W   *base;
            u64  offset;

            RelativePointer() : base(nullptr), offset(0) {}

            RelativePointer(W *base, u64 offset = 0) : base(base), offset(offset) {
                aassert(base != nullptr, "relative pointer base is null");
            }

            RelativePointer(RelativePointer const &other) {
                base = other.base;
                offset = other.offset;
            }

            RelativePointer(RelativePointer &&other) {
                base = other.base;
                offset = other.offset;
            }

            RelativePointer & operator=(RelativePointer const &other) {
                base = other.base;
                offset = other.offset;
                return *this;
            }

            RelativePointer & operator=(RelativePointer &&other) {
                base = other.base;
                offset = other.offset;
                return *this;
            }

            bool operator==(RelativePointer const &other) {
                return base == other.base && offset == other.offset;
            }

            bool operator==(RelativePointer &&other) {
                return base == other.base && offset == other.offset;
            }

            bool operator!=(RelativePointer const &other) {
                return !(*this == other);
            }

            bool operator!=(RelativePointer &&other) {
                return !(*this == other);
            }

            template<typename C>
            bool operator==(RelativePointer<C, W> const &other) {
                return base == other.base && offset == other.offset;
            }

            template<typename C>
            bool operator==(RelativePointer<C, W> &&other) {
                return base == other.base && offset == other.offset;
            }

            template<typename C>
            bool operator!=(RelativePointer<C, W> const &other) {
                return !(*this == other);
            }

            template<typename C>
            bool operator!=(RelativePointer<C, W> &&other) {
                return !(*this == other);
            }

            bool operator==(T const *&other) {
                return isValid() && (reinterpret_cast<T *>(base + offset) == other);
            }

            bool operator==(T *&&other) {
                return isValid() && (reinterpret_cast<T *>(base + offset) == other);
            }

            bool operator!=(T const *&other) {
                return !(*this == other);
            }

            bool operator!=(T *&&other) {
                return !(*this == static_cast<T *&&>(other));
            }

            operator bool() {
                return isValid();
            }

            T & operator *() {
                return *reinterpret_cast<T *>(base + offset);
            }

            T * operator->() {
                return reinterpret_cast<T *>(base + offset);
            }

            template<typename C>
            explicit operator C *() {
                static_assert(sizeof(C) <= sizeof(T), "cannot cast relative pointer to a type of higher size");
                return reinterpret_cast<C *>(base + offset);
            }

            template<typename C>
            explicit operator RelativePointer<C, W>() {
                static_assert(sizeof(C) <= sizeof(T), "cannot cast relative pointer to a type of higher size");
                return RelativePointer<C, W> {
                    base,
                    offset,
                };
            }

            bool isValid() {
                return base != nullptr;
            }
        };
    }
}

#endif

