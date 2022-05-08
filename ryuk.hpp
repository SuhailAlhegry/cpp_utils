#if !defined RYUK_HPP
#define RYUK_HPP

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include <stdio.h>
#include <utility>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using char_t = u8;

void printStack(void) {
    constexpr u64 MAX_STACK_COUNT = 64;
    void *stack[MAX_STACK_COUNT];
    u16 frames;
    SYMBOL_INFO *symbol;
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, nullptr, true);

    frames = CaptureStackBackTrace(0, 100, stack, nullptr);
    symbol = (SYMBOL_INFO *) calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    printf("=========call stack==========\n");
    for (int i = 1; i < frames; i++)
        {
            SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);

            printf("%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address);
        }
    printf("=============================\n");

    free(symbol);
}

// utils
#define between(a, min, max) ((a) >= (min) && (a) <= (max))

// assertions
inline bool assert_handler(const char *conditionCode, const char *file, int line, const char *report, ...) {
    char buffer[512 * 512];
    va_list args;
    va_start(args, report);
#if defined(_WIN32) // msvc, even though we tried shutting it up with _CRT_SECURE_NO_WARNINGS
    vsprintf_s(buffer, report, args);
#else
    vsprintf(buffer, report, args);
#endif
    printf("assertion raised: '%s' in '%s' at line %i failed\n", conditionCode, file, line);
    printf("%s\n", buffer);
    va_end(args);
    printStack();
    return true;
}

#define halt() (exit(1))

#if !defined(RELEASE)
    #define rassert(condition, ...) ((void)(!(condition) && assert_handler(#condition, __FILE__, __LINE__, __VA_ARGS__) && (halt(), 1)))
#else
#if defined(RELEASE_ASSERT)
    #define rassert(condition, report) ((void)(!(condition) && (halt(), 1)))
#endif
    #define rassert(condition, report) ((void)(condition))
#endif

// defer
template<typename F>
struct _Defer {
    F f;

    _Defer(F &&f) : f(std::move(f)) {}

    ~_Defer() {
        f();
    }  
};

template<typename F>
inline _Defer<F> _defer_func(F &&f) {
    return _Defer<F>(std::forward<F>(f));
}

#define macro_concat(x, y) x##y
#define macro_concat2(x, y) macro_concat(x, y)
#define defer(code) auto macro_concat2(_defer_, __COUNTER__) = _defer_func([&]()->void{ code; })

// code
namespace ryuk {
    namespace memory {
        typedef void *(*fn_allocator)(u64 size, u64 alignment, void *previousMemory, u64 previousSize);
        typedef void (*fn_deallocator)(void *memory, u64 size);

        inline size_t align(size_t size, size_t alignment) {
            return (size + (alignment - 1)) & ~(alignment - 1);
        }

        void *default_allocator(u64 size, u64 alignment = sizeof(u64), void *previousMemory = nullptr, u64 previousSize = 0) {
            if (size == previousSize) return previousMemory;
            u64 aligned = align(size, alignment);
            if (previousMemory == nullptr) {
                void *result = malloc(aligned);
                memset(result, 0, aligned);
                return result;
            }
            void *newMemory =  realloc(previousMemory, aligned);
            if (size > previousSize) {
                memset(((u8 *) newMemory) + previousSize, 0, size - previousSize);    
            }
            return newMemory;
        }

        void default_deallocator(void *mem, u64 size) {
            (void)size; // shutting off the compiler warning
            return free(mem);
        }

        template<typename T, fn_allocator allocator_f = default_allocator, fn_deallocator deallocator_f = default_deallocator>
        struct address final {
            using type = T;

            address() {
                this->ptr = reinterpret_cast<T *>(allocator_f(sizeof(T), sizeof(T), nullptr, 0));
                this->ptr->T();
            }

            address(T &&value) {
                this->ptr = reinterpret_cast<T *>(allocator_f(sizeof(T), sizeof(T), nullptr, 0));
                this->operator=(std::move(value));
            }

            address(const address &other) {
                this->ptr = other.ptr;
                this->isCopy = true;
            }

            address(address &&other) {
                this->ptr = other.ptr;
                other.ptr = nullptr;
            }
            
            address &operator=(const T &value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address &operator=(T &&value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = std::move(value);
                return *this;
            }
            
            address &operator=(const address &other) {
                rassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
                this->isCopy = true;
            }
            
            address &operator=(address &&other) {
                rassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
                other.ptr = nullptr;
            }

            bool operator ==(const address &other) const {
                return this->ptr == other.ptr;
            }

            bool operator !=(const address &other) const {
                return this->ptr != other.ptr;
            }

            bool operator ==(const address &&other) const {
                return this->ptr == other.ptr;
            }

            bool operator !=(const address &&other) const {
                return this->ptr != other.ptr;
            }

            operator T() const {
                rassert(this->ptr != nullptr, "trying to dereference an invalid address");
                return *reinterpret_cast<T*>(ptr);
            }

            T *const operator&() const {
                rassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            T *operator->() {
                rassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            void destroy() {
                rassert(!this->isCopy, "you can't destroy a copy of an address, try passing the address by reference");
                deallocator_f(this->ptr, sizeof(T));
                this->ptr = nullptr;
            }
        private:
            void *ptr;
            bool isCopy;
        };

        template<typename T>
        struct memory_view;

        template<typename T, fn_allocator allocator_f = default_allocator, fn_deallocator deallocator_f = default_deallocator>
        struct region final {
            using type = T;

            static constexpr region invalid() {
                return region {};
            }

            region(u64 length) {
                if (length == 0) {
                    length = 1;
                }
                this->_memory = reinterpret_cast<T *>(allocator_f(sizeof(T) * length, sizeof(T), nullptr, 0));
                this->_length = length;
            }

            region(const region &other) {
                rassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                this->isCopy = true;
            }

            region(region &&other) {
                rassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                other._memory = nullptr;
                other._length = 0;
            }

            region &operator=(const region &other) {
                if (this->_memory == other._memory) {
                    return *this; 
                }
                rassert(this->_memory == nullptr, "trying assign a new region to a region that is still valid");
                rassert(other._memory != nullptr, "trying to assign an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                this->isCopy = true;
            }

            region &operator=(const region &&other) {
                if (this->_memory == other._memory) {
                    return *this;
                }
                rassert(this->_memory == nullptr, "trying assign a new region to a region that is still valid");
                rassert(other._memory != nullptr, "trying to assign an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
            }

            bool operator ==(const region &other) const {
                return this->_memory == other._memory;
            }

            bool operator !=(const region &other) const {
                return this->_memory != other._memory;
            }

            bool operator ==(const region &&other) const {
                return this->_memory == other._memory;
            }

            bool operator !=(const region &&other) const {
                return this->_memory != other._memory;
            }

            void resize(u64 length) {
                rassert(!this->isCopy, "you can't resize a copy of a region, try passing the region by reference");
                
                this->_memory = reinterpret_cast<T *>(allocator_f(sizeof(T) * length, sizeof(T), this->_memory, this->_length * sizeof(T)));
                this->_length = length;
            }

            void destroy() {
                rassert(!this->isCopy, "you can't destroy a copy of a region, try passing the region by reference");
                deallocator_f(this->_memory, this->_length * sizeof(T));
                this->_memory = nullptr;
                this->_length = 0;
            }

            T &operator[](u64 index) const {
                rassert(index < _length, "region access out of bounds");
                return this->_memory[index];
            }

            T *const operator&() const {
                return this->_memory;
            }

            u64 length() const {
                return _length;
            }

            bool isValid() const {
                return this->_memory != nullptr && length() != 0;
            }

            memory_view<T> view(u64 low, u64 high) {
                rassert(isValid(), "trying to create a memory view from an invalid region");
                rassert(low >= 0, "trying to create a memory view with an invalid low bound");
                rassert(high > low, "trying to create a memory view with an invalid high bound");
                rassert(high < _length, "trying to create a memory view with an invalid high bound");
                return memory_view<T>(_memory, low, high);
            }
        private:
            region () {}
            T *_memory = nullptr;
            u64 _length = 0;
            bool isCopy = false;
        };

        template<typename T, u64 _length>
        struct static_region final {
            using type = T;
            static constexpr u64 slength = _length;
            static_assert(_length > 0, "cannot create a static region of length 0");
            
            static_region() {}

            static_region(const static_region &other) {
                if (other._data == this->_data) return;
                memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            static_region(const static_region &&other) {
                if (other._data == this->_data) return;
                memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            static_region operator=(const static_region &other) {
                if (other._data == this->_data) return;
                memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            static_region operator=(const static_region &&other) {
                if (other._data == this->_data) return;
                memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            bool operator ==(const static_region &other) const {
                return this->_data == other._data;
            }

            bool operator !=(const static_region &other) const {
                return this->_data != other._data;
            }
            bool operator ==(const static_region &&other) const {
                return this->_data == other._data;
            }

            bool operator !=(const static_region &&other) const {
                return this->_data != other._data;
            }

            T &operator[](u64 index) {
                rassert(index < _length, "static region access out of bounds");
                return _data[index];
            }

            T const * const operator&() const {
                return &(_data[0]);
            }

            constexpr u64 length() {
                return _length;
            }

            constexpr bool isValid() {
                return true;
            }

            memory_view<T> view(u64 low, u64 high) {
                rassert(low >= 0, "trying to create a memory view with an invalid low bound");
                rassert(high > low, "trying to create a memory view with an invalid high bound");
                rassert(high <= _length, "trying to create a memory view with an invalid high bound");
                return memory_view<T>(_data, low, high);
            }
        private:
            T _data[_length] = {0};
        };

        template<typename T>
        struct memory_view {
            using type = T;

            memory_view(
                T *memory,
                u64 low,
                u64 high
            ) : _memory(memory), _low(low), _high(high) {
                rassert(memory != nullptr, "trying to create a memory view with an invalid memory");
                rassert(low >= 0, "trying to create a memory view with an invalid low bound");
                rassert(high > low, "trying to create a memory view with an invalid high bound");
            }

            memory_view(
                const memory_view &other
            ) : _memory(other._memory), _low(other._low), _high(other._high) {
                rassert(_memory != nullptr, "trying to copy a memory view with an invalid memory");
                rassert(_low >= 0, "trying to copy a memory view with an invalid low bound");
                rassert(_high > _low, "trying to copy a memory view with an invalid high bound");
            }

            memory_view(
                memory_view &&other
            ) : _memory(other._memory), _low(other._low), _high(other._high) {
                rassert(_memory != nullptr, "trying to move a memory view with an invalid memory");
                rassert(_low >= 0, "trying to move a memory view with an invalid low bound");
                rassert(_high > _low, "trying to move a memory view with an invalid high bound");
                other._memory = nullptr;
                other._low = 0;
                other._high = 0;
            }

            memory_view &operator =(
                const memory_view &other
            ) {
                rassert(other._memory != nullptr, "trying to copy a memory view with an invalid memory");
                rassert(other._low >= 0, "trying to copy a memory view with an invalid low bound");
                rassert(other._high > _low, "trying to copy a memory view with an invalid high bound");
                _memory = other._memory;
                _low = other._low;
                _high = other._high;
            }

            memory_view &operator =(
                memory_view &&other
            ) {
                rassert(other._memory != nullptr, "trying to move a memory view with an invalid memory");
                rassert(other._low >= 0, "trying to move a memory view with an invalid low bound");
                rassert(other._high > _low, "trying to move a memory view with an invalid high bound");
                _memory = other._memory;
                _low = other._low;
                _high = other._high;
                other._memory = nullptr;
                other._low = 0;
                other._high = 0;
            }

            bool operator==(const memory_view &other) const {
                return _memory == other._memory && other._low == other._low && other._high == other._high;
            }

            bool operator!=(const memory_view &other) const {
                return _memory != other._memory || other._low != other._low || other._high != other._high;
            }

            bool operator==(const memory_view &&other) const {
                return _memory == other._memory && other._low == other._low && other._high == other._high;
            }

            bool operator!=(const memory_view &&other) const {
                return _memory != other._memory || other._low != other._low || other._high != other._high;
            }

            T &operator[](u64 index) {
                rassert(isValid(), "trying to access an invalid memory view");
                rassert(index < length(), "memory view access out of bounds");
                return _memory[_low + index];
            }
            
            T *const operator&() const {
                rassert(isValid(), "trying to take address of an invalid memory view");
                return &_memory[_low];
            }

            u64 length() const {
                return _high - _low;
            }

            bool isValid() const {
                return _memory != nullptr && _low >= 0 && _high > _low;
            }
        private:
            T *_memory;
            u64 _low;
            u64 _high;
        };

        template<typename T, fn_allocator allocator_f = default_allocator, fn_deallocator deallocator_f = default_deallocator>
        struct array {
            using type = T;
            using region_t = region<T, allocator_f, deallocator_f>;

            array(u64 capacity = 8) : _region(region_t(capacity)), _length(0) {}

            array(const array &other) {
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(other._region);
                _length = other._length;
            }

            array(array &&other) {
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(std::move(other._region));
                _length = other._length;
            }

            array &operator=(const array &other) {
                rassert(!_region.isValid(), "the underlying memory region of the target array is still valid, destroy the array first before assigning a new value");
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(other._region);
                _length = other._length;
            }

            array &operator=(array &&other) {
                rassert(!_region.isValid(), "the underlying memory region of the target array is still valid, destroy the array first before assigning a new value");
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(std::move(other._region));
                _length = other._length;
                other._length = 0;
            }

            bool operator==(const array &other) const {
                return _region == other._region;
            }

            bool operator!=(const array &other) const {
                return _region != other._region;
            }

            bool operator==(const array &&other) const {
                return _region == other._region;
            }

            bool operator!=(const array &&other) const {
                return _region != other._region;
            }

            T &operator[](u64 index) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                rassert(index < _length, "array access out of bounds");
                return _region[index];
            }

            T *const operator&() {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                return &_region;
            }

            void append(const T &value) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == _region.length()) {
                    _region.resize(_length * 2);
                }
                // NOTE: make sure it's still valid
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");

                _region[_length++] = value;
            }

            void append(T &&value) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == _region.length()) {
                    _region.resize(_length * 2);
                }
                // NOTE: make sure it's still valid
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");

                _region[_length++] = std::move(value);
            }

            T &pop() {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                return _region[--_length];
            }

            void swap(u64 aindex, u64 bindex) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                rassert(aindex < _length, "a index is out of bounds");
                rassert(bindex < _length, "b index is out of bounds");
                T temp = _region[aindex];
                _region[aindex] = _region[bindex];
                _region[bindex] = temp;
            }
            
            // TODO: normal remove
            T &swapRemove(u64 index) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                rassert(index < _length, "trying to remove an element from array with an out of bounds index");
                swap(index, _length - 1);
                return pop();
            }

            void reserve(u64 amount) {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                _region.resize(_region.length() + amount);
            }

            void shrinkToFit() {
                rassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == 0) {
                    _region.resize(1);
                } else {
                    _region.resize(_length);
                }
            }

            array copy() {
                array result(this->_length);
                memcpy(result._region._memory, this->_region._memory, this->_length * sizeof(T));
                result._length = this->_length;
                return result;
            }

            void destroy() {
                _region.destroy();
                _length = 0;
            }

            bool isValid() const {
                return _region.isValid();
            }

            u64 length() const {
                return _length;
            }

            u64 capacity() const {
                return _region.length();
            }

            memory_view<T> view(u64 low, u64 high) {
                return _region.view(low, high);
            }
        private:
            region_t _region;
            u64 _length = 0;
        };
        
        enum FileMode {
            FILE_BINARY,
            FILE_TEXT,
        };

        template<typename T, fn_allocator allocator_f = default_allocator, fn_deallocator deallocator_f = default_deallocator>
        region<T, allocator_f, deallocator_f> readfile(const char *path, FileMode mode = FILE_BINARY) {
            FILE* file = nullptr;
            constexpr u64 READ_BUFFER_SIZE = 256;

            const char *readMode = "rb";
            if (mode == FILE_TEXT) readMode = "r";

            if ((file = fopen(path, readMode)) != nullptr) {
                fseek(file, 0, SEEK_END);
                u64 fileSize = ftell(file);
                rewind(file);
                region<T, allocator_f, deallocator_f> memory(fileSize);
                u64 totalElementsRead = 0;
                T buffer[READ_BUFFER_SIZE];
                while (!feof(file)) {
                    u64 readBytes = fread(buffer, sizeof *buffer, READ_BUFFER_SIZE, file);
                    if (readBytes == 0) break;
                    for (int i = 0; i < readBytes; i++) {
                        memory[totalElementsRead++] = buffer[i];
                    }
                }
                fclose(file);
                return memory;
            }
            
            return region<T, allocator_f, deallocator_f>::invalid();
        }

        template<typename memory_holder_t>
        inline bool writeToFile(const char *path, memory_holder_t &holder, u64 elementsToWrite = 0, FileMode mode = FILE_BINARY) {
            rassert(holder.isValid(), "trying to write to file from an invalid memory holder");
            constexpr u64 typeSize = sizeof(typename memory_holder_t::type);
            u64 dataBytes = holder.length() * typeSize;
            rassert(holder.length() >= elementsToWrite, "trying to write more elements than stored in the memory holder");

            FILE *file = nullptr;

            const char *writeMode = "wb";
            if (mode == FILE_TEXT) writeMode = "w";

            if ((file = fopen(path, writeMode)) != nullptr) {
                u64 bytes = dataBytes;
                if (elementsToWrite != 0) {
                    bytes = elementsToWrite * typeSize;
                }
                fwrite(&holder, typeSize, bytes, file);
                fclose(file);
                return true;
            }

            return false;
        }
    }
}

#undef _CRT_SECURE_NO_DEPRECATE
#undef _CRT_SECURE_NO_WARNINGS
#endif