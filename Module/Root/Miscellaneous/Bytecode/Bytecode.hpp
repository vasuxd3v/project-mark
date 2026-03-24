#pragma once
#include "Includes.hpp"
#include <Common/include/Luau/Bytecode.h>
#include <zstd/zstd.h>

inline int ModuleScriptOffset = Offsets::External::Bytecode::ModuleScript;
inline int LocalScriptOffset = Offsets::External::Bytecode::LocalScript;

inline std::string ReadBytecode(uintptr_t addr) {
    uintptr_t str = addr + 0x10;
    size_t len = *(size_t*)(str + 0x10);
    size_t cap = *(size_t*)(str + 0x18);
    uintptr_t data_ptr = (cap > 0x0f) ? *(uintptr_t*)(str + 0x00) : str;
    return std::string(reinterpret_cast<const char*>(data_ptr), len);
}

inline std::string DecompressBytecode(const std::string& compressed) {
    uint8_t hash[4];
    memcpy(hash, compressed.data(), 4);

    const char* bcode_magic = "RSB1";
    for (auto i = 0; i < 4; i++)
    {
        hash[i] ^= bcode_magic[i];
        hash[i] -= i * 41;
    }

    std::vector<uint8_t> compressed_bcode(compressed.begin(), compressed.end());
    for (size_t i = 0; i < compressed_bcode.size(); i++)
        compressed_bcode[i] ^= hash[i % 4] + i * 41;

    int bcode_len;
    memcpy(&bcode_len, compressed_bcode.data() + 4, 4);

    std::vector<std::uint8_t> bcode(bcode_len);
    if (ZSTD_decompress(bcode.data(), bcode_len, compressed_bcode.data() + 8, compressed_bcode.size() - 8) != bcode_len) {
        return "";
    }
    else {
        return std::string(bcode.begin(), bcode.end());
    }

    return "";
}