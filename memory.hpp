#if !defined(ACHILLES_MEMORY_HPP)
#define ACHILLES_MEMORY_HPP

// this file depends on <cstring> for 'std::memcpy' and friends
// should be able to remove this dependency when providing own 'std::memcpy' implementation
#include <cstring>
#include <initializer_list>
#include "types.hpp"
#include "assert.hpp"

#define KB(s) ((s) * 1024ULL)
#define MB(s) ((s) * KB(1))
#define GB(s) ((s) * MB(1))

namespace achilles {
    namespace memory {
        typedef void *(*fn_allocator)(u64 size, u64 alignment, void *previousMemory, u64 previousSize);
        typedef void (*fn_deallocator)(void *memory, u64 size);

        inline size_t align(size_t size, size_t alignment) {
            return (size + (alignment - 1)) & ~(alignment - 1);
        }

        template<typename T, fn_allocator allocator_f, fn_deallocator deallocator_f, bool autoDestroy = true>
        struct address final {
            using type = T;

            address() {
                this->ptr = reinterpret_cast<T *>(allocator_f(sizeof(T), sizeof(T), nullptr, 0));
                new (this->ptr) T();
            }

            address(T &&value) {
                this->ptr = reinterpret_cast<T *>(allocator_f(sizeof(T), sizeof(T), nullptr, 0));
                this->operator=(static_cast<T &&>(value));
            }

            address(const address &other) {
                this->ptr = other.ptr;
                this->isCopy = true;
            }

            address(address &&other) {
                this->ptr = other.ptr;
                other.ptr = nullptr;
            }

            ~address() {
                if (autoDestroy) {
                    destroy();
                }
            }
            
            address &operator=(const T &value) {
                aassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = value;
                return *this;
            }
            
            address &operator=(T &&value) {
                aassert(this->ptr != nullptr, "trying to assign a value to an invalid address");
                *reinterpret_cast<T *>(this->ptr) = static_cast<T &&>(value);
                return *this;
            }
            
            address &operator=(const address &other) {
                aassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
                this->isCopy = true;
                return *this;
            }
            
            address &operator=(address &&other) {
                aassert(other.ptr != nullptr, "trying to assign an invalid address to a valid address");
                if (this->ptr == other.ptr) {
                    return *this;
                }
                this->ptr = other.ptr;
                other.ptr = nullptr;
                return *this;
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
                aassert(this->ptr != nullptr, "trying to dereference an invalid address");
                return *reinterpret_cast<T*>(ptr);
            }

            T *operator&() {
                aassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            T *operator->() {
                aassert(this->ptr != nullptr, "trying to take the pointer of invalid address");
                return reinterpret_cast<T*>(ptr);
            }

            void destroy() {
                aassert(!this->isCopy, "you can't destroy a copy of an address, try passing the address by reference");
                deallocator_f(this->ptr, sizeof(T));
                this->ptr = nullptr;
            }
        private:
            void *ptr;
            bool isCopy;
        };

        template<typename T>
        struct memory_view;

        template<typename T, fn_allocator allocator_f, fn_deallocator deallocator_f, bool autoDestroy = true>
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
                aassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                this->isCopy = true;
            }

            region(region &&other) {
                aassert(other._memory != nullptr, "trying to construct a region with an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                other._memory = nullptr;
                other._length = 0;
            }

            region &operator=(const region &other) {
                if (this->_memory == other._memory) {
                    return *this; 
                }
                aassert(this->_memory == nullptr, "trying assign a new region to a region that is still valid");
                aassert(other._memory != nullptr, "trying to assign an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                this->isCopy = true;
                return *this;
            }

            region &operator=(const region &&other) {
                if (this->_memory == other._memory) {
                    return *this;
                }
                aassert(this->_memory == nullptr, "trying assign a new region to a region that is still valid");
                aassert(other._memory != nullptr, "trying to assign an invalid region");
                this->_memory = other._memory;
                this->_length = other._length;
                return *this;
            }

            ~region() {
                if (autoDestroy) {
                    destroy();
                }
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
                aassert(!this->isCopy, "you can't resize a copy of a region, try passing the region by reference");
                
                this->_memory = reinterpret_cast<T *>(allocator_f(sizeof(T) * length, sizeof(T), this->_memory, this->_length * sizeof(T)));
                this->_length = length;
            }

            void destroy() {
                aassert(!this->isCopy, "you can't destroy a copy of a region, try passing the region by reference");
                deallocator_f(this->_memory, this->_length * sizeof(T));
                this->_memory = nullptr;
                this->_length = 0;
            }

            T &operator[](u64 index) const {
                aassert(index < _length, "region access out of bounds");
                return this->_memory[index];
            }

            T *operator&() {
                return this->_memory;
            }

            u64 length() const {
                return _length;
            }

            u64 size() const {
                return _length;
            }

            bool isValid() const {
                return this->_memory != nullptr && length() != 0;
            }

            memory_view<T> view(u64 low, u64 high) {
                aassert(isValid(), "trying to create a memory view from an invalid region");
                aassert(high > low, "trying to create a memory view with an invalid high bound");
                aassert(high <= _length, "trying to create a memory view with an invalid high bound");
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
                if (+other._data == +this->_data) return;
                std::memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            static_region(const static_region &&other) {
                if (+other._data == +this->_data) return;
                std::memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
            }

            static_region &operator=(const static_region &other) {
                if (+other._data == +this->_data) return *this;
                std::memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
                return *this;
            }

            static_region &operator=(const static_region &&other) {
                if (+other._data == +this->_data) return *this;
                std::memcpy((u8 *) this->_data, (u8 *) other._data, _length * sizeof(T));
                return *this;
            }

            bool operator ==(const static_region &other) const {
                return +this->_data == +other._data;
            }

            bool operator !=(const static_region &other) const {
                return +this->_data != +other._data;
            }
            bool operator ==(const static_region &&other) const {
                return +this->_data == +other._data;
            }

            bool operator !=(const static_region &&other) const {
                return +this->_data != +other._data;
            }

            T &operator[](u64 index) {
                aassert(index < _length, "static region access out of bounds");
                return _data[index];
            }

            T *operator&() const {
                return (T *const) _data;
            }

            constexpr u64 length() {
                return _length;
            }

            constexpr u64 size() {
                return _length;
            }

            constexpr bool isValid() {
                return true;
            }

            memory_view<T> view(u64 low, u64 high) {
                aassert(high > low, "trying to create a memory view with an invalid high bound");
                aassert(high <= _length, "trying to create a memory view with an invalid high bound");
                return memory_view<T>(_data, low, high);
            }
        private:
            T _data[_length]{};
        };

        template<typename T>
        struct memory_view {
            using type = T;

            memory_view(
                T *memory,
                u64 low,
                u64 high
            ) : _memory(memory), _low(low), _high(high) {
                aassert(memory != nullptr, "trying to create a memory view with an invalid memory");
                aassert(high > low, "trying to create a memory view with an invalid high bound");
            }

            memory_view(
                const memory_view &other
            ) : _memory(other._memory), _low(other._low), _high(other._high) {
                aassert(_memory != nullptr, "trying to copy a memory view with an invalid memory");
                aassert(_high > _low, "trying to copy a memory view with an invalid high bound");
            }

            memory_view(
                memory_view &&other
            ) : _memory(other._memory), _low(other._low), _high(other._high) {
                aassert(_memory != nullptr, "trying to move a memory view with an invalid memory");
                aassert(_high > _low, "trying to move a memory view with an invalid high bound");
                other._memory = nullptr;
                other._low = 0;
                other._high = 0;
            }

            memory_view &operator =(
                const memory_view &other
            ) {
                aassert(other._memory != nullptr, "trying to copy a memory view with an invalid memory");
                aassert(other._high > _low, "trying to copy a memory view with an invalid high bound");
                _memory = other._memory;
                _low = other._low;
                _high = other._high;
                return *this;
            }

            memory_view &operator =(
                memory_view &&other
            ) {
                aassert(other._memory != nullptr, "trying to move a memory view with an invalid memory");
                aassert(other._high > _low, "trying to move a memory view with an invalid high bound");
                _memory = other._memory;
                _low = other._low;
                _high = other._high;
                other._memory = nullptr;
                other._low = 0;
                other._high = 0;
                return *this;
            }

            bool operator==(const memory_view &other) const {
                return _memory == other._memory && _low == other._low && _high == other._high;
            }

            bool operator!=(const memory_view &other) const {
                return _memory != other._memory || _low != other._low || _high != other._high;
            }

            bool operator==(const memory_view &&other) const {
                return _memory == other._memory && _low == other._low && _high == other._high;
            }

            bool operator!=(const memory_view &&other) const {
                return _memory != other._memory || _low != other._low || _high != other._high;
            }

            T &operator[](u64 index) {
                aassert(isValid(), "trying to access an invalid memory view");
                aassert(index < length(), "memory view access out of bounds");
                return _memory[_low + index];
            }
            
            T *operator&() {
                aassert(isValid(), "trying to take address of an invalid memory view");
                return &_memory[_low];
            }

            u64 length() const {
                return _high - _low;
            }

            u64 size() const {
                return _high - _low;
            }

            bool isValid() const {
                return _memory != nullptr && _high > _low;
            }

            memory_view view(u64 low, u64 high) {
                aassert(isValid(), "trying to create a view to an invalid memory view");
                aassert(high <= _high, "trying to create a view with an out of bound high value");
                return memory_view {
                    _memory,
                    low,
                    high,
                };
            }
        private:
            T *_memory;
            u64 _low;
            u64 _high;
        };

        template<typename T>
        struct array_view;

        template<typename T, fn_allocator allocator_f, fn_deallocator deallocator_f, bool autoDestroy = true>
        struct array {
            using type = T;
            using region_t = region<T, allocator_f, deallocator_f, autoDestroy>;

            array(u64 capacity = 8) : _region(region_t(capacity)), _length(0) {}

            array(std::initializer_list<T> list) : array(list.size()) {
                for (const auto &e : list) {
                    append(static_cast<const T &>(e));
                }
            }

            array(const array &other) : _region(other._region), _length(other._length) {
                aassert(other.isValid(), "the underlying memory region of the copied array is invalid");
            }

            array(array &&other) : _region(other._region), _length(other._length) {
                aassert(other.isValid(), "the underlying memory region of the copied array is invalid");
            }

            array &operator=(const array &other) {
                aassert(!_region.isValid(), "the underlying memory region of the target array is still valid, destroy the array first before assigning a new value");
                aassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(other._region);
                _length = other._length;
                return *this;
            }

            array &operator=(array &&other) {
                aassert(!_region.isValid(), "the underlying memory region of the target array is still valid, destroy the array first before assigning a new value");
                aassert(other.isValid(), "the underlying memory region of the copied array is invalid");
                _region = region_t(static_cast<region_t &&>(other._region));
                _length = other._length;
                other._length = 0;
                return *this;
            }

            ~array() {
                if (autoDestroy) {
                    destroy();
                }
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
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                aassert(index < _length, "array access out of bounds");
                return _region[index];
            }

            T *operator&() {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                return &_region;
            }

            void append(const T &value) {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == _region.length()) {
                    _region.resize(_length * 2);
                }
                // NOTE: make sure it's still valid
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");

                _region[_length++] = value;
            }

            void append(T &&value) {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == _region.length()) {
                    _region.resize(_length * 2);
                }
                // NOTE: make sure it's still valid
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");

                _region[_length++] = static_cast<T &&>(value);
            }

            void push(const T &value) {
                append(static_cast<const T &>(value));
            }

            void push(T &&value) {
                append(static_cast<T &&>(value));
            }

            T &pop() {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                return _region[--_length];
            }

            void swap(u64 aindex, u64 bindex) {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                aassert(aindex < _length, "a index is out of bounds");
                aassert(bindex < _length, "b index is out of bounds");
                T temp = _region[aindex];
                _region[aindex] = _region[bindex];
                _region[bindex] = temp;
            }
            
            // TODO: normal remove
            T &swapRemove(u64 index) {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                aassert(index < _length, "trying to remove an element from array with an out of bounds index");
                swap(index, _length - 1);
                return pop();
            }

            void reserve(u64 amount) {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                _region.resize(_region.length() + amount);
            }

            void shrinkToFit() {
                aassert(_region.isValid(), "the underlying memory region of this array is invalid");
                if (_length == 0) {
                    _region.resize(1);
                } else {
                    _region.resize(_length);
                }
            }

            array copy() {
                array result(this->_length);
                std::memcpy(result._region._memory, this->_region._memory, this->_length * sizeof(T));
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

            u64 size() const {
                return _length;
            }

            memory_view<T> view(u64 low, u64 high) {
                aassert(_length > 0, "trying to create a view to an empty array");
                return _region.view(low, high);
            }

            array_view<T> arrayView() {
                aassert(isValid(), "trying to make a view of an invalid array");
                return array_view<T>(&_region, _length, _region.length());
            }
        private:
            region_t _region;
            u64 _length = 0;
        };

        template<typename T, u64 _size>
        struct static_array {
            using type = T;
            using region_t = static_region<T, _size>;

            static_array() {}

            static_array(std::initializer_list<T> list) {
                for (const auto &e : list) {
                    append(static_cast<const T &>(e));
                }
            }

            static_array(const static_array &other) {
                this->_region = static_cast<const region_t &>(other._region);
            }

            static_array(static_array &&other) {
                this->_region = static_cast<region_t &&>(other._region);
            }

            static_array &operator=(const static_array &other) {
                this->_region = static_cast<const region_t &>(other.region);
                return *this;
            }

            static_array operator=(static_array &&other) {
                this->_region = static_cast<region_t &&>(other.region);
                return *this;
            }

            bool operator==(const static_array &other) const {
                return this->_region == other._region;
            }

            bool operator!=(const static_array &other) const {
                return this->_region != other._region;
            }

            bool operator==(static_array &&other) const {
                return this->_region == other._region;
            }

            bool operator!=(static_array &&other) const {
                return this->_region != other._region;
            }

            T &operator[](u64 index) {
                return _region[index];
            }

            T *operator&() {
                return &_region;
            }

            void append(const T &value) {
                aassert(_length < _size, "static array is full");
                _region[_length++] = value;
            }

            void append(T &&value) {
                aassert(_length < _size, "static array is full");
                _region[_length++] = static_cast<T &&>(value);
            }

            void push(const T &value) {
                append(static_cast<const T &>(value));
            }

            void push(T &&value) {
                append(static_cast<T &&>(value));
            }

            T &pop() {
                aassert(_length > 0, "static array is empty");
                return _region[--_length];
            }

            void swap(u64 aindex, u64 bindex) {
                aassert(aindex < _length, "a index is out of bounds");
                aassert(bindex < _length, "b index is out of bounds");
                T temp = _region[aindex];
                _region[aindex] = _region[bindex];
                _region[bindex] = temp;
            }

            T &swapRemove(u64 index) {
                aassert(index < _length, "index out of bounds");
                swap(index, _length - 1);
                return pop();
            }

            constexpr bool isValid() const {
                return true;
            }

            u64 length() const {
                return _length;
            }

            constexpr u64 capacity() const {
                return _size;
            }

            constexpr u64 size() const {
                return _length;
            }

            memory_view<T> view(u64 low, u64 high) {
                aassert(_length > 0, "trying to create a view to an empty static array");
                return _region.view(low, high);
            }
            
            array_view<T> arrayView() {
                aassert(isValid(), "trying to make a view of an invalid static array");
                return array_view<T>(&_region, _length, _region.length());
            }
        private:
            static_region<T, _size> _region;
            u64 _length = 0;
        };
        
        template<typename T>
        struct array_view {
            using type = T;
            
            array_view(
                T *memory, u64 length, u64 capacity 
            ) :
                _memory(memory), _length(length), _capacity(capacity)
            {
                aassert(memory != nullptr, "the pointer used to create the array view is null");
                aassert(length < capacity, "length used to create the array view is invalid");
                aassert(capacity != 0, "capacity used to create the array view is invalid");
            }

            array_view(
                const array_view &other
            ) : _memory(other._memory), _length(other._length), _capacity(other._capacity) {
                aassert(_memory != nullptr, "the pointer used to create the array view is null");
                aassert(_length < _capacity, "length used to create the array view is invalid");
                aassert(_capacity != 0, "capacity used to create the array view is invalid");
            }

            array_view(
                array_view &&other
            ) : _memory(other._memory), _length(other._length), _capacity(other._capacity) {
                aassert(_memory != nullptr, "the pointer used to create the array view is null");
                aassert(_length < _capacity, "length used to create the array view is invalid");
                aassert(_capacity != 0, "capacity used to create the array view is invalid");
                other._memory = nullptr;
                other._length = 0;
                other._capacity = 0;
            }

            array_view &operator=(const array_view &other) {
                aassert(_memory != nullptr, "the pointer used to create the array view is null");
                aassert(_length < _capacity, "length used to create the array view is invalid");
                aassert(_capacity != 0, "capacity used to create the array view is invalid");
                _memory = other._memory;
                _length = other._length;
                _capacity = other._capacity;
                return *this;
            }

            array_view &operator=(array_view &&other) {
                aassert(_memory != nullptr, "the pointer used to create the array view is null");
                aassert(_length < _capacity, "length used to create the array view is invalid");
                aassert(_capacity != 0, "capacity used to create the array view is invalid");
                _memory = other._memory;
                _length = other._length;
                _capacity = other._capacity;
                other._memory = nullptr;
                other._length = 0;
                other._capacity = 0;
                return *this;
            }

            bool operator==(const array_view &other) const {
                return _memory == other._memory && _length == other._length && _capacity == other._capacity;
            }

            bool operator!=(const array_view &other) const {
                return _memory != other._memory || _length != other._length || _capacity != other._capacity;
            }

            bool operator==(array_view &&other) const {
                return _memory == other._memory && _length == other._length && _capacity == other._capacity;
            }

            bool operator!=(array_view &&other) const {
                return _memory != other._memory || _length != other._length || _capacity != other._capacity;
            }

            T &operator[](u64 index) {
                aassert(isValid(), "trying to access an invalid array view");
                aassert(index < length(), "array view access out of bounds");
                return _memory[index];
            }
            
            T *operator&() const {
                aassert(isValid(), "trying to take address of an invalid array view");
                return _memory;
            }

            u64 length() const {
                return _length;
            }

            u64 capacity() const {
                return _capacity;
            }

            u64 size() const {
                return _length;
            }

            bool isValid() const {
                return _memory != nullptr && _capacity > _length;
            }

            void append(const T &value) {
                aassert(_length < _capacity, "array view is full");
                _memory[_length++] = value;
            }

            void append(T &&value) {
                aassert(_length < _capacity, "array view is full");
                _memory[_length++] = static_cast<T &&>(value);
            }

            void push(const T &value) {
                append(static_cast<const T &>(value));
            }

            void push(T &&value) {
                append(static_cast<T &&>(value));
            }

            T &pop() {
                aassert(_length > 0, "array view is empty");
                return _memory[--_length];
            }

            void swap(u64 aindex, u64 bindex) {
                aassert(aindex < _length, "a index is out of bounds");
                aassert(bindex < _length, "b index is out of bounds");
                T temp = _memory[aindex];
                _memory[aindex] = _memory[bindex];
                _memory[bindex] = temp;
            }

            T &swapRemove(u64 index) {
                aassert(index < _length, "index out of bounds");
                swap(index, _length - 1);
                return pop();
            }

            memory_view<T> view(u64 low, u64 high) {
                aassert(isValid(), "trying to create a view to an invalid array view");
                aassert(_length > 0, "trying to create a view to an empty array view");
                aassert(low < high, "trying to create a view with an invalid low bound");
                aassert(high <= _length, "trying to create a view with an invalid high bound");

                return memory_view<T> {
                    _memory,
                    low,
                    high,
                };
            }
        private:
            T *_memory = nullptr;
            u64 _length = 0;
            u64 _capacity = 0;
        };
    }
}

#endif

