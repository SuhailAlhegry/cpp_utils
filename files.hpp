#include "memory.hpp"
#if !defined(ACHILLES_FILES_HPP)
#define ACHILLES_FILES_HPP

// this file requires <stdio.h> for 'fopen' and friends
#include <cstdio>
#include "types.hpp"
#include "assert.hpp"

namespace achilles {
    namespace files {
        using namespace memory;

        enum FileMode : u8 {
            FILE_BINARY,
            FILE_TEXT,
        };


        inline Block readFile(const char *path, Allocator &allocator, FileMode mode = FILE_BINARY) {
            FILE* file = nullptr;
            constexpr u64 READ_BUFFER_SIZE = 256;

            const char *readMode = "rb";
            if (mode == FILE_TEXT) readMode = "r";

            if ((file = std::fopen(path, readMode)) != nullptr) {
                std::fseek(file, 0, SEEK_END);
                u64 fileSize = ftell(file);
                std::rewind(file);
                Block blk = allocator.allocate(fileSize);
                u8 *memory = blk;
                u64 totalBytesRead = 0;
                u8 buffer[READ_BUFFER_SIZE];
                while (!std::feof(file)) {
                    u64 readBytes = std::fread(buffer, sizeof *buffer, READ_BUFFER_SIZE, file);
                    if (readBytes == 0) break;
                    for (u64 i = 0; i < readBytes; i++) {
                        memory[totalBytesRead++] = buffer[i];
                    }
                }
                std::fclose(file);
                return blk;
            }
            
            return Block { nullptr, 0 };
        }

        inline bool writeToFile(const char *path, Block &block, u64 elementsToWrite = 0, FileMode mode = FILE_BINARY) {
            aassert(block.isValid(), "trying to write to a file from an invalid memory block");
            u64 count = block.size;
            aassert(count >= elementsToWrite, "trying to write more elements than stored in the memory holder");

            FILE *file = nullptr;

            const char *writeMode = "wb";
            if (mode == FILE_TEXT) writeMode = "w";

            if ((file = std::fopen(path, writeMode)) != nullptr) {
                u64 elementCount = count;
                if (elementsToWrite != 0) {
                    elementCount = elementsToWrite;
                }
                std::fwrite(block, sizeof(u8), elementCount, file);
                std::fclose(file);
                return true;
            }

            return false;
        }
    }
}

#endif

