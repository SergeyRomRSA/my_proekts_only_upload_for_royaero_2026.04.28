#include <vector>
#include <cstdint>
#include <iostream>
#include <ctime>
#include "transaction_form.h"

struct HeaderBlock {
    std::vector<uint8_t> version;
    std::vector<uint8_t> prevBlock;
    std::vector<uint8_t> mercleRoot;
    std::vector<uint8_t> time;
    std::vector<uint8_t> bits;
    std::vector<uint8_t> nonce;
};

std::ostream& operator<<(std::ostream& os, const HeaderBlock& hex) {
    os << u8_to_hex(hex.version) << " "
       << u8_to_hex(hex.prevBlock) << " "
       << u8_to_hex(hex.mercleRoot) << " "
       << u8_to_hex(hex.time) << " "
       << u8_to_hex(hex.bits) << " "
       << u8_to_hex(hex.nonce);
    return os;
}

std::vector<uint8_t> get_info(const HeaderBlock& hex) {
    std::vector<uint8_t> out_vector;
    
    out_vector.reserve(
        hex.version.size() +
        hex.prevBlock.size() +
        hex.mercleRoot.size() +
        hex.time.size() +
        hex.bits.size() +
        hex.nonce.size()
    );

    out_vector.insert(out_vector.end(), hex.version.begin(), hex.version.end());
    out_vector.insert(out_vector.end(), hex.prevBlock.begin(), hex.prevBlock.end());
    out_vector.insert(out_vector.end(), hex.mercleRoot.begin(), hex.mercleRoot.end());
    out_vector.insert(out_vector.end(), hex.time.begin(), hex.time.end());
    out_vector.insert(out_vector.end(), hex.bits.begin(), hex.bits.end());
    out_vector.insert(out_vector.end(), hex.nonce.begin(), hex.nonce.end());

    return out_vector;
}
