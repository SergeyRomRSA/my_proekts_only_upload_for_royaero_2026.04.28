#include <iostream>
#include <vector>
#include <cstdint>

// --- Переворот в little-endian (txid в Bitcoin обычно отображается LE) ---
std::vector<uint8_t> reverse_bytes(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> rev(data.rbegin(), data.rend());
    return rev;
}

// --- Получение txid из raw-транзакции ---
std::vector<uint8_t> get_txid(const std::vector<uint8_t>& rawTx) {
    std::vector<uint8_t> hash = double_sha256(rawTx);
    return reverse_bytes(hash); // txid в формате LE
}
