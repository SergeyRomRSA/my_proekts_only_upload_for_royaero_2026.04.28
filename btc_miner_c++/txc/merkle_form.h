#include <vector>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "../utils.h"

// --- Склеивание двух хешей и двойной SHA256 ---
std::vector<uint8_t> merkle_pair_hash(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), a.begin(), a.end());
    combined.insert(combined.end(), b.begin(), b.end());
    return double_sha256(combined);
}

// --- Вычисление Merkle root ---
std::vector<uint8_t> compute_merkle_root(std::vector<std::vector<uint8_t>> tx_hashes) {
    if (tx_hashes.empty()) return std::vector<uint8_t>(32, 0); // пустой блок

    while (tx_hashes.size() > 1) {
        std::vector<std::vector<uint8_t>> next_level;
        for (size_t i = 0; i < tx_hashes.size(); i += 2) {
            if (i + 1 < tx_hashes.size()) {
                next_level.push_back(merkle_pair_hash(tx_hashes[i], tx_hashes[i+1]));
            } else {
                // дублируем последний хеш, если нечётное количество
                next_level.push_back(merkle_pair_hash(tx_hashes[i], tx_hashes[i]));
            }
        }
        tx_hashes = next_level;
    }

    return tx_hashes[0]; // Merkle root
}

// --- Вспомогательная функция для вывода hex ---
void print_hex(const std::vector<uint8_t>& data) {
    for (auto b : data) printf("%02x", b);
    printf("\n");
}

// --- Пример использования ---
int main() {
    // допустим, у нас 3 транзакции (txid в виде двойного SHA256)
    std::vector<uint8_t> tx1 = double_sha256(std::vector<uint8_t>{0x01});
    std::vector<uint8_t> tx2 = double_sha256(std::vector<uint8_t>{0x02});
    std::vector<uint8_t> tx3 = double_sha256(std::vector<uint8_t>{0x03});

    std::vector<std::vector<uint8_t>> tx_hashes = {tx1, tx2, tx3};
    std::vector<uint8_t> merkle_root = compute_merkle_root(tx_hashes);

    std::cout << "Merkle root: ";
    print_hex(merkle_root);
    return 0;
}
