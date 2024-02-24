#include "utils/assert.hpp"
#include <string.h>
#if !defined(ACHILLES_MEMORY_HPP)
#define ACHILLES_MEMORY_HPP

// this file depends on <cstring> for 'std::memcpy' and friends
// should be able to remove this dependency when providing own 'std::memcpy' implementation
#include <cstring>
#include <utility>
// require cstdlib for malloc, free
#include <cstdlib>
#include <initializer_list>
#include <type_traits>
#include "types.hpp"
#include "assert.hpp"

#define KB(s) ((s) * 1024ULL)
#define MB(s) ((s) * KB(1024ULL))
#define GB(s) ((s) * MB(1024ULL))

namespace achilles {
    namespace memory {
        struct Allocator {
            virtual u8 * allocate(u64 size) = 0;
            virtual bool tryResize(u8 **memory, u64 oldSize, u64 newSize) = 0;
            virtual void deallocate(u8 **memory, u64 size) = 0;
            virtual bool owns(u8 *const memory, u64 size) const { (void) memory, (void) size; return true; }
            virtual bool canAllocate(u64 size) const { (void) size; return true; }
            virtual bool canDeallocate(u8 *const memory, u64 size) const { (void) memory, (void) size; return true; }
        };

        struct NullAllocator : Allocator {
            u8 * allocate(u64 size) override {
                (void) size;
                return nullptr;
            }

            bool tryResize(u8 **memory, u64 oldSize, u64 newSize) override {
                (void) oldSize;
                (void) newSize;
                if (memory) *memory = nullptr;
                return false;
            }

            void deallocate(u8 **memory, u64 size) override {
                (void) size;
                if (memory) *memory = nullptr;
            }

            static NullAllocator &instance() {
                static auto _instance = NullAllocator{};
                return _instance;
            }
        };

        struct GlobalAllocator : Allocator {
            u8 * allocate(u64 size) override {
                u8 *result = (u8 *) malloc(size);
                if (result) {
                    memset(result, 0, size);
                }
                return result;
            }

            bool tryResize(u8 **memory, u64 oldSize, u64 newSize) override {
                if (memory == nullptr || *memory == nullptr) return false;
                u8 *result = (u8 *) realloc(*memory, newSize);
                if (result == nullptr) return false;
                if (oldSize < newSize) {
                } else {
                    u64 diff = newSize - oldSize;
                    u8 *pos = result + oldSize;
                    memset(pos, 0, diff);
                }
                *memory = result;
                return true;
            }

            void deallocate(u8 **memory, u64 size) override {
                (void) size;
                if (memory == nullptr || *memory == nullptr) return; 
                free(*memory);
                *memory = nullptr;
            }

            static GlobalAllocator &instance() {
                static auto _instance = GlobalAllocator{};
                return _instance;
            }
        };

        struct Block {
            Block(u8 *memory, u64 size, Allocator &allocator) : _memory{memory}, _size{size}, _allocator{allocator} {}

            Block(Block &&other) : _memory{other._memory}, _size{other._size}, _allocator{other._allocator} {
                other.invalidate();
            }

            Block &operator =(Block &&other) {
                if (isValid()) _allocator.deallocate(&_memory, _size);
                _memory = other._memory;
                _size = other._size;
                _allocator = other._allocator;
                other.invalidate();
                return *this;
            }

            ~Block() {
                _allocator.deallocate(&_memory, _size);
                invalidate();
            }

            Block(Block const &other) = delete;
            Block &operator =(Block const &other) = delete;

            bool operator ==(Block const &other) {
                return other._memory == _memory && other._size == _size;
            }

            bool operator ==(Block &&other) {
                return other._memory == _memory && other._size == _size;
            }

            template<typename T>
            explicit operator T*() const {
                return (T *) _memory;
            }

            bool isValid() const {
                return _memory != nullptr && _size > 0;
            }

            Block copy(Allocator &allocator) {
                u8 *result = allocator.allocate(_size);
                memcpy(_memory, _memory, _size);
                return Block{result, _size, allocator};
            }

            bool tryResize(u64 newSize) {
                if (_allocator.tryResize(&_memory, _size, newSize)) {
                    _size = newSize;
                    return true;
                } else  {
                    return false;
                }
            }

            u64 size() const {
                return _size;
            }
        private:
            void invalidate() {
                _memory = nullptr;
                _size = 0;
            }

            u8 *_memory;
            u64 _size;
            Allocator &_allocator;
        };

        template<typename T>
        struct Address {
            Address(Block &&block) : _block { std::move(block) } {}

            template<typename... Args>
            Address(Allocator &allocator, Args &&...args) {
                u8 *memory = allocator.allocate(sizeof(T));
                if (memory) {
                    new (memory) T { std::forward<Args>(args)... };
                }
                _block = Block{memory, sizeof(T), allocator};
            }

            Address(Address &&other) : _block{std::move(other._block)} {}

            Address & operator=(Address &&other) {
                _block = std::move(other._block);
                return *this;
            }

            Address(Address const &other) = delete;
            Address & operator =(Address const &other) = delete;

            explicit operator T*() const {
                return (T *) _block;
            }
            
            template<typename TR>
            operator TR*() const {
                static_assert(
                    std::is_base_of_v<TR, T> || std::is_base_of_v<T, TR> ||
                    std::is_convertible_v<T*, TR*> || std::is_convertible_v<TR*, T*>,
                    "cannot cast to a pointer with incompatible types, if you want to force it, use 'as'"
                );
                return (TR *) _block;
            }

            template<typename TR>
            TR *as() const {
                return (TR *) _block;
            }

            T *operator->() const {
                aassert(isValid(), "dereferncing an invalid address");
                T *ptr = (T *) _block;
                return ptr;
            }

            T & operator*() const {
                aassert(isValid(), "dereferncing an invalid address");
                T *ptr = (T *) _block;
                return *ptr;
            }

            template<typename TR>
            operator Address<TR> () {
                static_assert(
                    std::is_base_of_v<TR, T> || std::is_base_of_v<T, TR> ||
                    std::is_convertible_v<T*, TR*> || std::is_convertible_v<TR*, T*>,
                    "cannot cast addresses with incompatible types, if you want to force it, use 'convert'"
                );
                if (isValid()) {
                    auto result = Address<TR> {
                        std::move(_block),
                    };
                    return result;
                } else {
                    return nullptr;
                }
            }

            template<typename TR>
            Address<TR> as() {
                if (isValid()) {
                    auto result = Address<TR> {
                        std::move(_block),
                    };
                    return result;
                } else {
                    return nullptr;
                }
            }

            Address copy(Allocator &allocator) {
                return Address{_block.copy(allocator)};
            }

            bool isValid() const {
                return _block.isValid();
            }
        private:
            Block _block;
        };

        template<typename T>
        struct Slice {
            Slice(T *memory, u64 size) : _memory{memory}, _size{size} {}
            Slice(Slice const &other) = default;
            Slice(Slice &&other) = default;
            Slice &operator=(Slice const &other) = default;
            Slice &operator=(Slice &&other) = default;

            template<u64 SIZE>
            Slice<char const>(char const (&str)[SIZE]) : _memory{&str}, _size{SIZE} {}

            Slice<char const>(char const *_str) : _memory{(char *) _str}, _size{strlen(_str)} {}

            bool operator==(Slice const &other) const {
                if (_memory == other.memory && _size == other._size) {
                    return true;
                } else if (_size == other._size) {
                    bool areEqual = true;
                    for (auto i = 0; i < _size && areEqual; ++i) {
                        areEqual &= get(i) == other.get(i);
                    }
                    return areEqual;
                } else {
                    return false;
                }
            }

            bool operator!=(Slice const &other) const {
                return !operator==((Slice const &) other);
            }

            bool operator==(Slice &&other) const {
                if (_memory == other.memory && _size == other._size) {
                    return true;
                } else if (_size == other._size) {
                    bool areEqual = true;
                    for (auto i = 0; i < _size && areEqual; ++i) {
                        areEqual &= get(i) == other.get(i);
                    }
                    return areEqual;
                } else {
                    return false;
                }
            }

            bool operator!=(Slice &&other) const {
                return !operator==((Slice &&) other);
            }

            operator T *() const {
                return (T *) _memory;
            }

            bool isValid() const {
                return _memory != nullptr && _size > 0;
            }

            u64 size() const { return _size; }

            T & get(u64 index) const {
                aassert(isValid(), "trying to access an invalid slice");
                aassert(index <= _size && index >= 0, "slice index out of range");
                return _memory[index];
            }

            T & operator[](u64 index) const {
                return get(index);
            }

            Slice slice(u64 low, u64 high) const {
                aassert(low < high && high <= _size, "invalid slice range for slice");
                return Slice {
                    _memory + low,
                    high - low,
                };
            }

            Slice<T> slice(u64 low) {
                aassert(low < _size, "invalid slice range for slice");
                return Slice<T> {
                    _memory + low,
                    _size - low,
                };
            }
        private:
            T *_memory;
            u64 _size;
        };

        template<typename T>
        struct Array {
            Array(Allocator &allocator = NullAllocator::instance(), u64 capacity = 8) {
                if (capacity > 0) {
                    u8 *memory = allocator.allocate(capacity * sizeof(T));
                    if (memory) {
                        _block = Block { memory, capacity * sizeof(T), allocator };
                    }
                }
            }

            template<typename... Items>
            Array(Allocator &allocator, T &&item, Items &&...items) {
                std::initializer_list<T> items_ = {std::forward<T>(item), std::forward<Items>(items)...};
                size_t size_ = items_.size() * sizeof(T);
                u8 *memory = allocator.allocate(size_);
                if (memory) {
                    _block = Block { memory, size_, allocator };
                    for (auto &i : items_) {
                        // this stupid hack because we cannot have a non-const iterator when iterating std::initializer_list
                        auto &e = const_cast<T &>(i);
                        push(std::move(e));
                    }
                }
            }

            Array(Array &&other)
                : _block {std::move(other._block)},
                  _size {other._size}
            {
                other._size = 0;
            }

            Array & operator=(Array &&other) {
                _block = std::move(other._block);
                _size = other._size;
                other._size = 0;
                return *this;
            }

            u64 size() const {
                return _size;
            }

            u64 capacity() const {
                return _block.size() / sizeof(T);
            }

            bool isValid() const {
                return _block.isValid();
            }

            bool push(T &&value) {
                if (!isValid()) return false;
                if (capacity() == _size) {
                    if (!_block.tryResize(_block.size() * 2)) {
                        return false;
                    }
                }
                T *memory = (T *) _block;
                if constexpr (std::is_trivial_v<T>) {
                    memory[_size++] = value;
                } else {
                    new (memory + _size) T(std::move(value));
                    _size += 1;
                }
                return true;
            }

            bool push(T const &value) {
                if (!isValid()) return false;
                if (capacity() == _size) {
                    if (!_block.tryResize(_block.size() * 2)) {
                        return false;
                    }
                }
                T *memory = (T *) _block;
                if constexpr (std::is_trivial_v<T>) {
                    memory[_size] = value;
                } else {
                    new (memory + _size) T(value);
                }
                _size += 1;
                return true;
            }

            T & pop() {
                aassert(isValid(), "popping from an invalid array");
                aassert(_size > 0, "popping from an empty array");
                T *memory = (T *) _block;
                return memory[--_size];
            }

            T & top() {
                T *memory = (T *) _block;
                return memory[_size - 1];
            }

            T & get(u64 index) const {
                aassert(isValid(), "getting a value from an invalid array");
                aassert(_size > 0, "getting a value from an empty array");
                T *memory = (T *) _block;
                return memory[index];
            }

            T & operator[](u64 index) const {
                return get(index);
            }

            operator bool() {
                return isValid();
            }

            T remove(u64 index) {
                aassert(isValid(), "removing a value from an invalid array");
                aassert(_size > 0, "removing a value from an empty array");
                aassert(index >= 0 && index < _size, "Array.remove: index out of bound");
                T *memory = (T *) _block;
                T item = memory[index];
                for (auto i = index + 1; i < _size; ++i) {
                    memory[i - 1] = memory[i];
                }
                _size -= 1;
                return item;
            }

            void swap(u64 first, u64 second) {
                aassert(isValid(), "swaping values in an invalid array");
                aassert(_size > 0, "swaping values of an empty array");
                aassert(first >= 0 && first < _size, "Array.swap: first index out of bound");
                aassert(second >= 0 && second < _size, "Array.swap: second index out of bound");
                aassert(first != second, "Array.swap: first and second are the same!");
                T *memory = (T *) _block;
                T temp = memory[first];
                memory[first] = memory[second];
                memory[second] = temp;
            }

            // removes the element by swapping it with the last element
            T swapRemove(u64 index) {
                aassert(isValid(), "removing a value from an invalid array");
                aassert(_size > 0, "removing a value from an empty array");
                aassert(index >= 0 && index < _size, "Array.swapRemove: index out of bound");
                swap(index, --_size);
                T *memory = (T *) _block;
                return memory[_size];
            }

            u64 find(T value) const {
                aassert(isValid(), "trying to find a value from an invalid array");
                if (_size == 0) return U64_MAX;
                T *memory = (T *) _block;
                for (auto i = 0; i < _size; ++i) {
                    if (value == memory[i]) {
                        return i;
                    }
                }
                return U64_MAX;
            }

            void clear() {
                _size = 0;
            }

            ~Array() {
                if (!isValid()) return;
                T *mem = (T *) _block;
                for (auto i = 0; i < _size; ++i) {
                    mem[i].~T();
                }
                clear();
            }

            Block & operator &() {
                return _block;
            }

            Slice<T> slice(u64 low, u64 high) {
                aassert(low < high && high <= _size, "invalid slice range for array");
                return Slice<T> {
                    ((T *) _block) + low,
                    high - low,
                };
            }

            Slice<T> slice(u64 low) {
                aassert(low < _size, "invalid slice range for array");
                return Slice<T> {
                    ((T *) _block) + low,
                    _size - low,
                };
            }

            Slice<T> slice() {
                return Slice<T> {
                    (T *) _block,
                    _size,
                };
            }
        private:
            Block _block;
            u64 _size = 0;
        };

        template<typename T, typename W>
        struct RelativePointer {
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

        private:
            W   *base;
            u64  offset;
        };

        template<typename T>
        struct OffsetPointer {
            OffsetPointer(u64 offset) : _offset(offset) {}

            explicit operator T*() {
                return asPtr();
            }

            T * operator->() {
                return asPtr();
            }

            T &operator *() {
                return *(asPtr());
            }

            T **operator&() {
                return &(asPtr());
            }

            operator bool() {
                return isValid();
            }

            bool isValid() const {
                return _offset != 0;
            }
        private:
            T *asPtr() {
                return isValid() ? *reinterpret_cast<T *>(((char *) &_offset) + _offset) : nullptr;
            }
            u64 _offset;
        };
    }
}

#endif

