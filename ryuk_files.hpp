#ifndef RYUK_FILES_H
#define RYUK_FILES_H

#include <stdio.h>
#include "ryuk_memory.hpp"
#include "ryuk_types.hpp"

enum FileMode {
    FILE_BINARY,
    FILE_TEXT,
};

inline MemoryAddress<char_t> readfile(const char *path, FileMode mode = FILE_BINARY, Allocator<char_t> allocator = DefaultAllocator<char_t>{}) {
    FILE* file = nullptr;
    constexpr size_t READ_BUFFER_SIZE = 256;

    const char *readMode = "rb";
    if (mode == FILE_TEXT) readMode = "r";

    if (fopen_s(&file, path, readMode) == 0) {
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        rewind(file);
        MemoryAddress<char_t> memory = allocator.alloc(fileSize);
        size_t totalBytesRead = 0;
        char_t buffer[READ_BUFFER_SIZE];
        while (!feof(file)) {
            size_t readBytes = fread(buffer, sizeof *buffer, READ_BUFFER_SIZE, file);
            if (readBytes == 0) break;
            for (int i = 0; i < readBytes; i++) {
                memory[totalBytesRead++] = buffer[i];
            }
        }
        fclose(file);
        return memory;
    }
    
    return MemoryAddress<char_t>{};
}

inline bool writefile(const char *path, MemoryAddress<char_t> data, FileMode mode = FILE_BINARY) {
    rassert(data.ptr != nullptr && data.size != 0, "trying to write to file from an invalid address");
    FILE *file = nullptr;

    const char *writeMode = "wb";
    if (mode == FILE_TEXT) writeMode = "w";

    if (fopen_s(&file, path, writeMode)) {
        fwrite(data.ptr, sizeof(char_t), data.size, file);
        fclose(file);
        return true;
    }

    return false;
}

#endif