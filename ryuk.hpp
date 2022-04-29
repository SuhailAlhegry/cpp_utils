#include <corecrt_memcpy_s.h>
#if !defined (RYUK_HPP)
#define RYUK_HPP

#include <stdio.h>
#include <utility>
#include <inttypes.h>
#include <string.h>

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

inline bool assert_handler(const char *report, const char *conditionCode, const char *file, int line) {
    printf("assertion raised: '%s', '%s' in '%s' at line %i failed\n", report, conditionCode, file, line);
    return true;
}

#define halt() (exit(1))

#if !defined(RELEASE)
#define rassert(condition, report) ((void)(!(condition) && assert_handler(report, #condition, __FILE__, __LINE__) && (halt(), 1)))
#else
#define rassert(condition, report) (((void)sizeof(condition)),halt())
#endif

// defer
template<typename F>
struct _Defer {
    F f;

    _Defer(F &&f) : f(std::forward<F>(f)) {}

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

namespace ryuk {
    namespace memory {
        using _regionImpl = void;
        using _address_t = void;

        struct default_region_allocator final {
            _regionImpl *allocate(u64 tsize, u64 length, u64 alignment);
            _regionImpl *reallocate(_regionImpl *region, u64 tsize, u64 length, u64 alignment);
            void         deallocate(_regionImpl *region);
            void        *getUnderlyingPtr(_regionImpl *region);
            u64          getUnderlyingLength(_regionImpl *region);
        };

        struct default_address_allocator final {
            _address_t *allocate(u64 tsize, u64 alignment);      
            void        deallocate(_address_t *ptr);
        };

        template<typename T, class allocator_t = default_address_allocator>
        struct address final {
            using type = T;

            address() {
                this->ptr = allocator.allocate(sizeof(T), sizeof(T));
            }

            address(T &&value) {
                this->ptr = allocator.allocate(sizeof(T), sizeof(T));
                this->operator=(value);
            }

            address(address<T> &other) {
                this->ptr = other.ptr;
                this->isCopy = true;
            }

            address(address<T> &&other) {
                this->ptr = other.ptr;
            }
            
            address<T> &operator=(T &value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address<T> &operator=(T &&value) {
                rassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address<T> &operator=(address<T> &other) {
                rassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
                this->isCopy = true;
            }
            
            address<T> &operator=(address<T> &&other) {
                rassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
            }

            bool operator ==(address<T> &other) {
                return this->ptr == other.ptr;
            }

            bool operator !=(address<T> &other) {
                return this->ptr != other.ptr;
            }

            bool operator ==(address<T> &&other) {
                return this->ptr == other.ptr;
            }

            bool operator !=(address<T> &&other) {
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
                printf("destroying address \n");
                allocator.deallocate(this->ptr);
                this->ptr = nullptr;
            }
        private:
            allocator_t allocator{};
            _address_t *ptr;
            bool isCopy;
        };

        template<typename region_t>
        struct memory_view;

        template<typename T, class allocator_t = default_region_allocator>
        struct region final {
            using type = T;

            static constexpr region<T> invalid() {
                return region<T> {};
            }

            region(u64 length) {
                if (length == 0) {
                    length = 1;
                }
                this->impl = allocator.allocate(sizeof(T), length, sizeof(T));
            }

            region(region<T> &other) {
                rassert(other.impl != nullptr, "trying to construct a region with an invalid region");
                this->impl = other.impl;
                this->isCopy = true;
            }

            region(region<T> &&other) {
                rassert(other.impl != nullptr, "trying to construct a region with an invalid region");
                this->impl = other.impl;
            }

            region<T> &operator=(region<T> &other) {
                if (this->impl == other.impl) {
                    return *this; 
                }
                rassert(this->impl == nullptr, "trying assign a new region to a region that is still valid");
                rassert(other.impl != nullptr, "trying to assign an invalid region");
                this->impl = other.impl;
                this->isCopy = true;
            }

            region<T> &operator=(region<T> &&other) {
                if (this->impl == other.impl) {
                    return *this;
                }
                rassert(this->impl == nullptr, "trying assign a new region to a region that is still valid");
                rassert(other.impl != nullptr, "trying to assign an invalid region");
                this->impl = other.impl;
            }

            bool operator ==(region<T> &other) {
                return this->impl == other.impl;
            }

            bool operator !=(region<T> &other) {
                return this->impl != other.impl;
            }

            bool operator ==(region<T> &&other) {
                return this->impl == other.impl;
            }

            bool operator !=(region<T> &&other) {
                return this->impl != other.impl;
            }

            void resize(u64 length) {
                rassert(!this->isCopy, "you can't resize a copy of a region, try passing the region by reference");
                printf("resizing region \n");
                this->impl = allocator.reallocate(this->impl, sizeof(T), length, sizeof(T));
            }

            void destroy() {
                rassert(!this->isCopy, "you can't destroy a copy of a region, try passing the region by reference");
                printf("destroying region \n");
                allocator.deallocate(this->impl);
                this->impl = nullptr;
            }

            T &operator[](u64 index) {
                rassert(index < length(), "region access out of bounds");
                T *ptr = reinterpret_cast<T *>(allocator.getUnderlyingPtr(this->impl));
                return ptr[index];
            }

            T *const operator&() {
                T *ptr = reinterpret_cast<T *>(allocator.getUnderlyingPtr(this->impl));
                return ptr;
            }

            u64 length() {
                return allocator.getUnderlyingLength(impl);
            }

            bool isValid() {
                return this->impl != nullptr && length() != 0;
            }

            memory_view<region> view(u64 low, u64 high) {
                return memory_view<region>(*this, low, high);
            }
        private:
            region<T> () {}
            allocator_t allocator{};
            _regionImpl *impl = nullptr;
            bool isCopy = false;
        };

        template<typename T, u64 _length>
        struct static_region final {
            using type = T;
            static constexpr u64 slength = _length;
            static_assert(_length > 0, "cannot create a static region of length 0");
            
            static_region() {}

            static_region(static_region<T, _length> &other) {
                memcpy_s(this->data, _length, other.data, _length);
            }

            static_region(static_region<T, _length> &&other) {
                memcpy_s(this->data, _length, other.data, _length);
            }

            static_region<T, _length> operator=(static_region<T, _length> &other) {
                memcpy_s(this->data, _length, other.data, _length);
            }

            static_region<T, _length> operator=(static_region<T, _length> &&other) {
                memcpy_s(this->data, _length, other.data, _length);
            }

            bool operator ==(static_region<T, _length> &other) {
                return this->data == other.data;
            }

            bool operator !=(static_region<T, _length> &other) {
                return this->data != other.data;
            }
            bool operator ==(static_region<T, _length> &&other) {
                return this->data == other.data;
            }

            bool operator !=(static_region<T, _length> &&other) {
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
                }
                _region.resize(_length);
            }

            void destroy() {
                _region.destroy();
                _length = 0;
            }

            bool isValid() {
                return _region.isValid();
            }

            u64 length() {
                return _length;
            }

            u64 capacity() {
                return _region.length();
            }

            memory_view<region_t> view(u64 low, u64 high) {
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

            if (fopen_s(&file, path, readMode) == 0) {
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

            if (fopen_s(&file, path, writeMode) == 0) {
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

#endif