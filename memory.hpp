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
                other.invalidate();
            }

            Block &operator =(Block &&other) {
                aassert(!isValid(), "trying to assign a new value to an already valid block, deallocate the block first.");
                memory = other.memory;
                size = other.size;
                other.invalidate();
                return *this;
            }

            #if defined(ACHILLES_ENABLE_DESTRUCTOR_LEAK_DETECTION)
            ~Block() {
                aassert(memory == nullptr, "memory leak");
            }
            #else
            Block(Block const &other) {
                memory = other.memory;
                size = other.size;
            }

            Block &operator =(Block const &other) {
                aassert(!isValid(), "trying to assign a new value to an already valid block, deallocate the block first.");
                memory = other.memory;
                size = other.size;
                return *this;
            }
            #endif

            template<typename T>
            operator T*() const {
                return (T *) memory;
            }

            bool isValid() const {
                return memory != nullptr && size > 0;
            }

            void invalidate() {
                memory = nullptr;
                size = 0;
            }

            u8 *memory;
            u64 size;
        };

        template<typename T>
        struct Address {
            Address(Block &&block) : _memory{ nullptr, 0 } {
                aassert(block.size >= sizeof(T), "T is larger than this address's memory block");
                _memory = (Block &&) block;
            }

            Address(void *ptr) : _memory { nullptr, 0 } {
                aassert(ptr == nullptr, "assigning a valid raw pointer to address");
            }

            Address(Address &&other) : _memory { (Block &&) other._memory } {}

            Address & operator=(Address &&other) {
                _memory = (Block &&) other._memory;
                return *this;
            }

            #if defined(ACHILLES_ENABLE_DESTRUCTOR_LEAK_DETECTION)
            ~Address() {
                aassert(!_memory.isValid(), "address memory leak");
            }
            #else
            Address(Address const &other) : _memory { (Block const &) other._memory } {}

            Address & operator =(Address const &other) {
                aassert(!isValid(), "assigning a new value to a valid address");
                _memory = (Block const &) other._memory;
                return *this;
            }
            #endif

            operator Block&() {
                return _memory;
            }

            T * operator->() const {
                return _memory;
            }

            operator T*() const {
                return _memory;
            }
            
            template<typename TR>
            operator TR*() const {
                static_assert(
                    std::is_base_of_v<TR, T> || std::is_base_of_v<T, TR> ||
                    std::is_convertible_v<T*, TR*> || std::is_convertible_v<TR*, T*>,
                    "cannot cast to a pointer with incompatible types, if you want to force it, use 'cast'"
                );
                return _memory;
            }

            template<typename TR>
            TR *as() const {
                return _memory;
            }

            T & operator*() const {
                aassert(isValid(), "dereferncing an invalid address");
                T *ptr = _memory;
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
                        (Block &&) _memory,
                    };
                    return result;
                } else {
                    return nullptr;
                }
            }

            template<typename TR>
            Address<TR> convert() {
                if (isValid()) {
                    auto result = Address<TR> {
                        std::move(_memory),
                    };
                    return result;
                } else {
                    return nullptr;
                }
            }

            bool isValid() const {
                return _memory.isValid();
            }
        private:
            Block _memory;
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
            using type = T;

            Array(Allocator *allocator, u64 capacity = 8) : _allocator{allocator} {
                aassert(allocator != nullptr, "using a null allocator to initialize an array");
                if (capacity > 0) {
                    _block = allocator->allocate(capacity * sizeof(type));
                }
            }

            Array(Array &&other)
                : _allocator {other._allocator},
                  _block {(Block &&) other._block},
                  _size {other._size}
            {
                other._size = 0;
                other._allocator = nullptr;
            }

            Array & operator=(Array &&other) {
                aassert(!isValid(), "trying to assign a new value to a valid array, destroy the array first.");
                _allocator = other._allocator;
                _block = other._block;
                _size = other._size;
                return *this;
            }

            u64 size() const {
                return _size;
            }

            u64 capacity() const {
                return _block.size / sizeof(type);
            }

            bool isValid() const {
                return _allocator != nullptr && _block.isValid();
            }

            bool push(type &&value) {
                if (!isValid()) return false;
                if (capacity() == _size) {
                    if (!_allocator->tryResize(_block, _block.size * 2)) {
                        return false;
                    }
                }
                type *memory = _block;
                if constexpr (std::is_trivial_v<T>) {
                    memory[_size++] = value;
                } else {
                    new (memory + _size) T(std::move(value));
                    _size += 1;
                }
                return true;
            }

            bool push(type const &value) {
                if (!isValid()) return false;
                if (capacity() == _size) {
                    if (!_allocator->tryResize(_block, _block.size * 2)) {
                        return false;
                    }
                }
                type *memory = _block;
                if constexpr (std::is_trivial_v<T>) {
                    memory[_size] = value;
                } else {
                    new (memory + _size) T(value);
                }
                _size += 1;
                return true;
            }

            type & pop() {
                aassert(isValid(), "popping from an invalid array");
                aassert(_size > 0, "popping from an empty array");
                type *memory = _block;
                return memory[--_size];
            }

            type & top() {
                type *memory = _block;
                return memory[_size - 1];
            }

            type & get(u64 index) const {
                aassert(isValid(), "getting a value from an invalid array");
                aassert(_size > 0, "getting a value from an empty array");
                type *memory = _block;
                return memory[index];
            }

            type & operator[](u64 index) const {
                return get(index);
            }

            T remove(u64 index) {
                aassert(isValid(), "removing a value from an invalid array");
                aassert(_size > 0, "removing a value from an empty array");
                aassert(index >= 0 && index < _size, "Array.remove: index out of bound");
                T *memory = _block;
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
                T *memory = _block;
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
                T *memory = _block;
                return memory[_size];
            }

            u64 find(T value) {
                aassert(isValid(), "trying to find a value from an invalid array");
                aassert(_size > 0, "trying to find a value from an empty array");
                T *memory = _block;
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

            bool destroy() {
                if (!isValid()) return false;
                _allocator->deallocate(_block);
                clear();
                return true;
            }

            Block & operator &() {
                return _block;
            }

            Slice<T> slice(u64 low, u64 high) {
                aassert(low < high && high <= _size, "invalid slice range for array");
                return Slice<T> {
                    (T *) (_block.memory + low),
                    high - low,
                };
            }

            Slice<T> slice(u64 low) {
                aassert(low < _size, "invalid slice range for array");
                return Slice<T> {
                    (T *) (_block.memory + low),
                    _size - low,
                };
            }

            Slice<T> slice() {
                return Slice<T> {
                    (T *) _block.memory,
                    _size,
                };
            }
        private:
            Allocator *_allocator;
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

