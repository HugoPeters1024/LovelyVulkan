#pragma once
#include "precomp.h"

static std::vector<char> readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        logger::error("Could not open compute shader binary {}", filepath);
        exit(1);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), static_cast<long>(fileSize));
    file.close();
    return buffer;
}
