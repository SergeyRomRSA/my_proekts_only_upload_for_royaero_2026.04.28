#include <vector>
#include <cstdint>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

std::vector<uint8_t> i32_to_u8(const uint32_t& word, bool little_endian = true) {
    std::vector<uint8_t> bytes(4);
    int a,b,c,d;

    if (little_endian) a = 0, b = 1, c = 2, d = 3;
    else a = 3, b = 2, c = 1, d = 0;
    
    bytes[a] = (word >> 24) & 0xFF;
    bytes[b] = (word >> 16) & 0xFF;
    bytes[c] = (word >>  8) & 0xFF;
    bytes[d] = (word >>  0) & 0xFF;
    return bytes;
}

std::vector<uint8_t> i64_to_u8(const uint64_t& word, bool little_endian = true) {
    std::vector<uint8_t> bytes(8);
    int a,b,c,d,e,f,g,h;

    if (little_endian) a = 0, b = 1, c = 2, d = 3, e = 4, f = 5, g = 6, h = 7;
    else a = 7, b = 6, c = 5, d = 4, e = 3, f = 2, g = 1, h = 0;
    
    bytes[a] = (word >> 56) & 0xFF;
    bytes[b] = (word >> 48) & 0xFF;
    bytes[c] = (word >> 40) & 0xFF;
    bytes[d] = (word >> 32) & 0xFF;
    bytes[e] = (word >> 24) & 0xFF;
    bytes[f] = (word >> 16) & 0xFF;
    bytes[g] = (word >>  8) & 0xFF;
    bytes[h] = (word >>  0) & 0xFF;
    return bytes;
}

std::vector<uint32_t> u8_to_u32(const std::vector<uint8_t>& bytes, bool little_endian = true) {
    uint32_t n = bytes.size() / 4;
    int a,b,c,d;
    if (bytes.size() % 4 != 0) throw std::runtime_error("u8_to_u32: размер не кратен 4 ");
    std::vector<uint32_t> out_vector(n);

    if (little_endian) a = 0, b = 1, c = 2, d = 3;
    else a = 3, b = 2, c = 1, d = 0;

    for (size_t i = 0; i < n; ++i) {
        out_vector[i] =
            (uint32_t(bytes[4*i + a]) << 24) |
            (uint32_t(bytes[4*i + b]) << 16) |
            (uint32_t(bytes[4*i + c]) <<  8) |
            (uint32_t(bytes[4*i + d]) <<  0);
    }
    return out_vector;
}

std::vector<uint8_t> u32_to_u8(const std::vector<uint32_t>& words, bool little_endian = true) {
    std::vector<uint8_t> out_vector(words.size() * 4);
    int a,b,c,d;

    if (little_endian) a = 0, b = 1, c = 2, d = 3;
    else a = 3, b = 2, c = 1, d = 0;

    for (size_t i = 0; i < words.size(); ++i) {
        out_vector[4*i + a] = (words[i] >> 24) & 0xFF;
        out_vector[4*i + b] = (words[i] >> 16) & 0xFF;
        out_vector[4*i + c] = (words[i] >>  8) & 0xFF;
        out_vector[4*i + d] = (words[i] >>  0) & 0xFF;
    }
    return out_vector;
}

uint8_t hex_value(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 0xA;
    if ('A' <= c && c <= 'F') return c - 'A' + 0xA;
    throw std::runtime_error("hex_value: неверный символ ");
}

std::vector<uint8_t> hex_to_u8(const std::string& hex) {
    uint32_t n = hex.size() / 2;
    if (hex.size() % 2 != 0) throw std::runtime_error("hex_to_u8: размер не кратен 2 ");

    std::vector<uint8_t> out_vector(n);
    
    for (size_t i = 0; i < n; ++i) {
        out_vector[i] =
            (hex_value(hex[2*i]) << 4) |
             hex_value(hex[2*i + 1]);
    }
    return out_vector;
}

std::vector<uint32_t> hex_to_u32(const std::string& hex) {
    auto bytes = hex_to_u8(hex);
    return u8_to_u32(bytes);
}

std::vector<uint8_t> reverse_u8(const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> out_vector(bytes.size());
    size_t n = bytes.size() - 1;
    for (size_t i = 0; i <= n; i++) out_vector[n-i] = bytes[i];
    return out_vector;
}

uint32_t reverse_i32(const uint32_t& word) {
    uint32_t rev = 
        (word & 0xFF) << 24 |
        ((word >> 8) & 0xFF) << 16 |
        ((word >> 16) & 0xFF) << 8 |
        ((word >> 24) & 0xFF);
    return rev;
}

std::string u32_to_hex(const std::vector<uint32_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (uint32_t v : data) {
        oss << std::setw(8) << v;
    }
    return oss.str();
}

std::string u8_to_hex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (uint8_t v : data) {
        oss << std::setw(2) << (int)v;
    }
    return oss.str();
}

template <size_t N>
std::string u32_to_hex_array(const std::array<uint32_t, N>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (uint32_t v : data) {
        oss << std::setw(8) << v;
    }
    return oss.str();
}

std::vector<uint8_t> bits_to_target(uint32_t bits) {
    std::vector<uint8_t> target(32, 0);

    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0xFFFFFF;

    if (exponent <= 3) {
        mantissa >>= 8*(3 - exponent);
        target[31] = mantissa & 0xFF;
        target[30] = (mantissa >> 8) & 0xFF;
        target[29] = (mantissa >> 16) & 0xFF;
    } else {
        size_t offset = 32 - exponent;
        target[offset]     = (mantissa >> 16) & 0xFF;
        target[offset + 1] = (mantissa >> 8) & 0xFF;
        target[offset + 2] = mantissa & 0xFF;
    }
    return target;
}

std::vector<uint8_t> create_u8(size_t size, uint8_t u = 0x00) {
    std::vector<uint8_t> vector(size, u);
    return vector;
}

// --- Хеш двойного SHA256 ---
std::vector<uint8_t> double_sha256(const std::vector<uint8_t>& data) {
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash1);
    SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);
    return std::vector<uint8_t>(hash2, hash2 + SHA256_DIGEST_LENGTH);
}

std::vector<uint8_t> string_to_u8(const std::string& frase) {
    size_t n = frase.size();
    std::vector<uint8_t> out_vector(n);
    for (size_t i = 0; i < n; i ++) out_vector[i] = (uint8_t)frase[i];
    return out_vector;
}

std::vector<uint8_t> merge_vector(const std::vector<uint8_t>& data1, const std::vector<uint8_t>& data2) {
    std::vector<uint8_t> out_vector;
    out_vector.reserve(data1.size() + data2.size());
    out_vector.insert(out_vector.end(), data1.begin(), data1.end());
    out_vector.insert(out_vector.end(), data2.begin(), data2.end());
    return out_vector;
}