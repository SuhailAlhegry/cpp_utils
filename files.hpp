#if !defined(ACHILLES_FILES_HPP)
#define ACHILLES_FILES_HPP

// this file requires <stdio.h> for 'fopen' and friends
#include <cstdio>
#include "types.hpp"
#include "assert.hpp"

namespace achilles {
    namespace files {
        enum FileMode {
            FILE_BINARY,
            FILE_TEXT,
        };

        template<typename T, memory::fn_allocator allocator_f, memory::fn_deallocator deallocator_f, bool autoDestroy>
        memory::region<T, allocator_f, deallocator_f, autoDestroy> readfile(const char *path, FileMode mode = FILE_BINARY) {
            FILE* file = nullptr;
            constexpr u64 READ_BUFFER_SIZE = 256;
            constexpr u64 typeSize = sizeof(T);

            const char *readMode = "rb";
            if (mode == FILE_TEXT) readMode = "r";

            if ((file = std::fopen(path, readMode)) != nullptr) {
                std::fseek(file, 0, SEEK_END);
                u64 fileSize = ftell(file) / typeSize;
                std::rewind(file);
                memory::region<T, allocator_f, deallocator_f> memory(fileSize);
                u64 totalBytesRead = 0;
                T buffer[READ_BUFFER_SIZE];
                while (!std::feof(file)) {
                    u64 readBytes = std::fread(buffer, sizeof *buffer, READ_BUFFER_SIZE, file);
                    if (readBytes == 0) break;
                    for (int i = 0; i < readBytes; i++) {
                        memory[totalBytesRead++] = buffer[i];
                    }
                }
                std::fclose(file);
                return memory;
            }
            
            return memory::region<T, allocator_f, deallocator_f>::invalid();
        }

        template<typename memory_holder_t>
        inline bool writeToFile(const char *path, memory_holder_t &holder, u64 elementsToWrite = 0, FileMode mode = FILE_BINARY) {
            aassert(holder.isValid(), "trying to write to file from an invalid memory holder");
            constexpr u64 typeSize = sizeof(typename memory_holder_t::type);
            u64 count = holder.length();
            aassert(count >= elementsToWrite, "trying to write more elements than stored in the memory holder");

            FILE *file = nullptr;

            const char *writeMode = "wb";
            if (mode == FILE_TEXT) writeMode = "w";

            if ((file = std::fopen(path, writeMode)) != nullptr) {
                u64 elementCount = count;
                if (elementsToWrite != 0) {
                    elementCount = elementsToWrite;
                }
                std::fwrite(&holder, typeSize, elementCount, file);
                std::fclose(file);
                return true;
            }

            return false;
        }
    }
}

#endif

