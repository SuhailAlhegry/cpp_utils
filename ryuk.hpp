#if !defined RYUK_HPP
#define RYUK_HPP

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <utility>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>

#if defined (_WIN32)
    #include <Windows.h>
#else
    #include <stdlib.h>
#endif

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

// utils
#define between(a, min, max) ((a) >= (min) && (a) <= (max))

// assertions
inline bool assert_handler(const char *conditionCode, const char *file, int line, const char *report, ...) {
    char buffer[512 * 512];
    va_list args;
    va_start(args, report);
    vsprintf(buffer, report, args);
    printf("assertion raised: '%s' in '%s' at line %i failed\n", conditionCode, file, line);
    printf("%s", buffer);
    va_end(args);
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

// allocation wrappers
inline void *ryuk_malloc(u64 size) {
#if defined (_WIN32)
  return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#else
  void *mem = malloc(size);
  memset(mem, 0, size);
  return mem;
#endif
}

inline void *ryuk_realloc(void *mem, u64 size) {
#if defined (_WIN32)
  return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
#else
  return realloc(mem, size);
#endif
}

inline void ryuk_free(void *mem) {
#if defined (_WIN32)
  HeapFree(GetProcessHeap(), 0, mem);
#else
  return free(mem);
#endif
}

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

template<u8 *ptr, u8 value, u64 size>
constexpr u8* const_memset() {
    for (u64 i = 0; i < size; ++i) {
        ptr[++i] = value;
    }
    return ptr;
}

// code
namespace ryuk {
    namespace memory {
        inline size_t align(size_t size, size_t alignment) {
            return (size + (alignment - 1)) & ~(alignment - 1);
        }

        struct default_region_allocator final {
            void *allocate(u64 tsize, u64 length, u64 alignment) {
                u64 totalsize = tsize * length;
                totalsize = align(totalsize, alignment);
                void *mem = ryuk_malloc(totalsize);
                return mem;
            }

            void *reallocate(void *region, u64 tsize, u64 length, u64 alignment) {
                if (region == nullptr) return nullptr;

                u64 totalsize = tsize * length;
                totalsize = align(totalsize, alignment);
                void *mem = ryuk_realloc(region, totalsize);

                return mem;
            }

            void deallocate(void *region) {
                if (region != nullptr) {
                    ryuk_free(region);
                }
            }
        };

        struct default_address_allocator final {
            void *allocate(u64 tsize, u64 alignment) {
                tsize = align(tsize, alignment);
                void *mem = ryuk_malloc(tsize);
                return mem;
            }

            void deallocate(void *address) {
                if (address != nullptr) {
                    ryuk_free(address);
                }
            }
        };

        template<typename T, class allocator_t = default_address_allocator>
        struct address final {
            using type = T;

            address() {
                this->ptr = reinterpret_cast<T *>(allocator.allocate(sizeof(T), sizeof(T)));
            }

            address(T &&value) {
                this->ptr = reinterpret_cast<T *>(allocator.allocate(sizeof(T), sizeof(T)));
                this->operator=(value);
            }

            address(address &other) {
                this->ptr = other.ptr;
                this->isCopy = true;
            }

            address(address &&other) {
                this->ptr = other.ptr;
            }
            
            address &operator=(T &value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address &operator=(T &&value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address &operator=(address &other) {
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
            }

            bool operator ==(address &other) {
                return this->ptr == other.ptr;
            }

            bool operator !=(address &other) {
                return this->ptr != other.ptr;
            }

            bool operator ==(address &&other) {
                return this->ptr == other.ptr;
            }

            bool operator !=(address &&other) {
                return this->ptr != other.ptr;
            }

            operator T() {
                rassert(this->ptr != nullptr, "trying to dereference an invalid address");
                return *reinterpret_cast<T*>(ptr);
            }

            T *const operator&() {
                rassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            T *operator->() {
                rassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            void destroy() {
                rassert(!this->isCopy, "you can't destroy a copy of an address, try passing the address by reference");
                allocator.deallocate(this->ptr);
                this->ptr = nullptr;
            }
        private:
            allocator_t allocator{};
            void *ptr;
            bool isCopy;
        };

        template<typename region_t>
        struct memory_view;

        template<typename T, class allocator_t = default_region_allocator>
        struct region final {
            using type = T;

            static constexpr region invalid() {
                return region {};
            }

            region(u64 length) {
                if (length == 0) {
                    length = 1;
                }
                this->_memory = reinterpret_cast<T *>(allocator.allocate(sizeof(T), length, sizeof(T)));
                this->_length = length;
            }

            region(const region &other) {
                rassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                this->isCopy = true;
            }

            region(const region &&other) {
                rassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
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
                this->_memory = reinterpret_cast<T *>(allocator.reallocate(this->_memory, sizeof(T), length, sizeof(T)));
                this->_length = length;
            }

            void destroy() {
                rassert(!this->isCopy, "you can't destroy a copy of a region, try passing the region by reference");
                allocator.deallocate(this->_memory);
                this->_memory = nullptr;
                this->_length = 0;
            }

            T &operator[](u64 index) const {
                rassert(index < _length, "region access out of bounds");
                return this->_memory[index];
            }

            T *const operator&() {
                return this->_memory;
            }

            u64 length() const {
                return _length;
            }

            bool isValid() const {
                return this->_memory != nullptr && length() != 0;
            }

            memory_view<region> view(u64 low, u64 high) const {
                return memory_view<region>(*this, low, high);
            }
        private:
            region () {}
            allocator_t allocator{};
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

            static_region(static_region &other) {
                if (other.data == this->data) return;
                memcpy(this->data, other.data, _length * sizeof(T));
            }

            static_region(static_region &&other) {
                if (other.data == this->data) return;
                memcpy(this->data, other.data, _length * sizeof(T));
            }

            static_region operator=(static_region &other) {
                if (other.data == this->data) return;
                memcpy(this->data, other.data, _length * sizeof(T));
            }

            static_region operator=(static_region &&other) {
                if (other.data == this->data) return;
                memcpy(this->data, other.data, _length * sizeof(T));
            }

            bool operator ==(static_region &other) {
                return this->data == other.data;
            }

            bool operator !=(static_region &other) {
                return this->data != other.data;
            }
            bool operator ==(static_region &&other) {
                return this->data == other.data;
            }

            bool operator !=(static_region &&other) {
                return this->data != other.data;
            }

            T &operator[](u64 index) {
                rassert(index < _length, "static region access out of bounds");
                return data[index];
            }

            T *const operator&() {
                return data;
            }

            // NOTE: make this work with memory_view
            constexpr u64 length() {
                return _length;
            }

            // NOTE: make this work with memory_view
            constexpr bool isValid() {
                return true;
            }

            memory_view<static_region> view(u64 low, u64 high) {
                return memory_view<static_region>(*this, low, high);
            }
        private:
            T data[_length]{};
        };

        template<typename region_t>
        struct memory_view {
            memory_view(
                region_t &region,
                u64 low,
                u64 high
            ) : region(region), low(low), high(high) {
                rassert(region.isValid(), "trying to create a memory view with an invalid region");
                rassert(low >= 0, "trying to create a memory view with an invalid low bound");
                rassert(high > low, "trying to create a memory view with an invalid high bound");
            }

            memory_view(
                memory_view &other
            ) : region(other.region), low(other.low), high(other.high) {
                rassert(region.isValid(), "trying to copy a memory view with an invalid region");
                rassert(low >= 0, "trying to copy a memory view with an invalid low bound");
                rassert(high > low, "trying to copy a memory view with an invalid high bound");
            }

            memory_view(
                memory_view &&other
            ) : region(other.region), low(other.low), high(other.high) {
                rassert(region.isValid(), "trying to move a memory view with an invalid region");
                rassert(low >= 0, "trying to move a memory view with an invalid low bound");
                rassert(high > low, "trying to move a memory view with an invalid high bound");
            }

            memory_view &operator =(
                memory_view &other
            ) {
                rassert(other.region.isValid(), "trying to copy a memory view with an invalid region");
                rassert(other.low >= 0, "trying to copy a memory view with an invalid low bound");
                rassert(other.high > low, "trying to copy a memory view with an invalid high bound");
                region = other.region;
                low = other.low;
                high = other.high;
            }

            memory_view &operator =(
                memory_view &&other
            ) {
                rassert(other.region.isValid(), "trying to copy a memory view with an invalid region");
                rassert(other.low >= 0, "trying to copy a memory view with an invalid low bound");
                rassert(other.high > low, "trying to copy a memory view with an invalid high bound");
                region = other.region;
                low = other.low;
                high = other.high;
            }

            bool operator==(memory_view &other) {
                return region == other.region && other.low == other.low && other.high == other.high;
            }

            bool operator!=(memory_view &other) {
                return region != other.region || other.low != other.low || other.high != other.high;
            }

            bool operator==(memory_view &&other) {
                return region == other.region && other.low == other.low && other.high == other.high;
            }

            bool operator!=(memory_view &&other) {
                return region != other.region || other.low != other.low || other.high != other.high;
            }

            typename region_t::type &operator[](u64 index) {
                rassert(index < length(), "memory view access out of bounds");
                return region[low + index];
            }
            
            typename region_t::type *const operator&() {
                return &region;
            }

            u64 length() {
                return high - low;
            }
        private:
            region_t &region;
            u64 low;
            u64 high;
        };

        template<typename T, class allocator_t = default_region_allocator>
        struct array {
            using type = T;
            using region_t = region<T, allocator_t>;

            array(u64 capacity = 8) : _region(region_t(capacity)), _length(0) {}

            array(array &other) {
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(other._region);
                _length = other._length;
            }

            array(array &&other) {
                rassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(std::move(other._region));
                _length = other._length;
            }

            array &operator=(array &other) {
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
            }

            bool operator==(array &other) {
                return _region == other._region;
            }

            bool operator!=(array &other) {
                return _region != other._region;
            }

            bool operator==(array &&other) {
                return _region == other._region;
            }

            bool operator!=(array &&other) {
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

            void append(T &value) {
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

            memory_view<region_t> view(u64 low, u64 high) const {
                rassert(high < _length, "trying to create a view to an array with a high bound that exceeds the array length");
                return _region.view(low, high);
            }
        private:
            region<T, allocator_t> _region;
            u64 _length = 0;
        };
        
        enum FileMode {
            FILE_BINARY,
            FILE_TEXT,
        };

        template<typename T, class allocator_t = default_region_allocator>
        region<T, allocator_t> readfile(const char *path, FileMode mode = FILE_BINARY) {
            FILE* file = nullptr;
            constexpr u64 READ_BUFFER_SIZE = 256;

            const char *readMode = "rb";
            if (mode == FILE_TEXT) readMode = "r";

            if ((file = fopen(path, readMode)) != nullptr) {
                fseek(file, 0, SEEK_END);
                u64 fileSize = ftell(file);
                rewind(file);
                region<T, allocator_t> memory(fileSize);
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
            
            return region<T, allocator_t>::invalid();
        }

        template<typename memory_holder_t>
        inline bool writeToFile(const char *path, memory_holder_t data, u64 elementsToWrite = 0, FileMode mode = FILE_BINARY) {
            rassert(data.isValid(), "trying to write to file from an invalid memory holder");
            constexpr u64 typeSize = sizeof(typename memory_holder_t::type);
            u64 dataBytes = data.length() * typeSize;
            rassert(data.length() >= elementsToWrite, "trying to write more elements than stored in the memory holder");

            FILE *file = nullptr;

            const char *writeMode = "wb";
            if (mode == FILE_TEXT) writeMode = "w";

            if ((file = fopen(path, writeMode)) != nullptr) {
                u64 bytes = dataBytes;
                if (elementsToWrite != 0) {
                    bytes = elementsToWrite * typeSize;
                }
                fwrite(&data, typeSize, bytes, file);
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