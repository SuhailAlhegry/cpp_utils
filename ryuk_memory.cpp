#include "ryuk.hpp"

namespace ryuk {
    namespace memory {
        struct _region {
            u64 length;
        };

        inline size_t align(size_t size, size_t alignment) {
            return (size + (alignment - 1)) & ~(alignment - 1);
        }

        _regionImpl *default_region_allocator::allocate(u64 tsize, u64 length, u64 alignment) {
            u64 totalsize = sizeof(u64) + tsize * length;
            totalsize = align(totalsize, alignment);
            void *mem = malloc(totalsize);
            memset(mem, 0, totalsize);
            ((_region *)mem)->length = length;
            return mem;
        }

        _regionImpl *default_region_allocator::reallocate(_regionImpl *region, u64 tsize, u64 length, u64 alignment) {
            if (region == nullptr) return nullptr;

            u64 oldsize = sizeof(u64) + ((_region *) region)->length * tsize;
            oldsize = align(oldsize, alignment);
            u64 totalsize = sizeof(u64) + tsize * length;
            totalsize = align(totalsize, alignment);
            void *mem = realloc(region, totalsize);
            
            if (oldsize < totalsize) {
                memset(((u8 *) mem) + oldsize, 0, totalsize - oldsize);
            }

            ((_region *)mem)->length = length;
            return mem;
        }

        void default_region_allocator::deallocate(_regionImpl *region) {
            if (region != nullptr) {
                free(region);
            }
        }

        void *default_region_allocator::getUnderlyingPtr(_regionImpl *region) {
            if (region == nullptr) return nullptr;
            return ((u8 *) region) + sizeof(u64);
        }

        u64 default_region_allocator::getUnderlyingLength(_regionImpl *region) {
            if (region == nullptr) return 0;
            return ((_region *) region)->length;
        }

        _address_t *default_address_allocator::allocate(u64 tsize, u64 alignment) {
            u64 totalsize = sizeof(u64) + tsize;
            totalsize = align(totalsize, alignment);
            void *mem = malloc(totalsize);
            memset(mem, 0, totalsize);
            return mem;
        }

        void default_address_allocator::deallocate(_address_t *address) {
            if (address != nullptr) {
                free(address);
            }
        }
    }
}