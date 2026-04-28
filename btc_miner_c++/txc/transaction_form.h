#include <vector>
#include <cstdint>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include "../miner/double_sha256_mining.h"


struct TxInput {
    std::vector<uint8_t> txid;
    std::vector<uint8_t> vout; // Vector Output // le
    uint8_t scriptSigSize;
    std::vector<std::vector<uint8_t>> scriptSig;
    std::vector<uint8_t> sequence; // le
};

std::vector<uint8_t> merge_TxInput(const TxInput& data) {
    size_t n = data.txid.size() + data.vout.size() + 1 + data.scriptSigSize + data.sequence.size();
    if (data.scriptSig.size() == 1) n --;
    std::vector<uint8_t> out_vector;
    out_vector.reserve(n);
    out_vector.insert(out_vector.end(), data.txid.begin(), data.txid.end());
    out_vector.insert(out_vector.end(), data.vout.begin(), data.vout.end());
    out_vector.insert(out_vector.end(), data.scriptSigSize);
    if (data.scriptSig.size() == 1) out_vector.insert(out_vector.end(), data.scriptSig[0].begin(), data.scriptSig[0].end());
    else {
        for (auto d : data.scriptSig){
            out_vector.insert(out_vector.end(), d.size());
            out_vector.insert(out_vector.end(), d.begin(), d.end());
        }
    }
    
    out_vector.insert(out_vector.end(), data.sequence.begin(), data.sequence.end());

    return out_vector;
}

std::vector<uint8_t> merge_TxInputs(const std::vector<TxInput>& data) {
    size_t n = 0;
    std::vector<std::vector<uint8_t>> tmp_vector(data.size());
    for (size_t i = 0; i < data.size(); i ++){
        tmp_vector[i] = merge_TxInput(data[i]);
        n += tmp_vector[i].size();
    }
    std::vector<uint8_t> out_vector;
    out_vector.reserve(n);
    for (auto t : tmp_vector) out_vector.insert(out_vector.end(), t.begin(), t.end());
    return out_vector;
}

struct TxOutput {
    std::vector<uint8_t> amount; //le
    uint8_t scriptPubKeySize;
    std::vector<uint8_t> scriptPubKey;
};

std::vector<uint8_t> merge_TxOutput(const TxOutput& data) {
    size_t n = data.amount.size() + 1 + data.scriptPubKeySize;
    std::vector<uint8_t> out_vector;
    out_vector.reserve(n);
    out_vector.insert(out_vector.end(), data.amount.begin(), data.amount.end());
    out_vector.insert(out_vector.end(), data.scriptPubKeySize);
    out_vector.insert(out_vector.end(), data.scriptPubKey.begin(), data.scriptPubKey.end());
    return out_vector;
}

std::vector<uint8_t> merge_TxOutputs(const std::vector<TxOutput>& data) {
    size_t n = 0;
    std::vector<std::vector<uint8_t>> tmp_vector(data.size());
    for (size_t i = 0; i < data.size(); i ++){
        tmp_vector[i] = merge_TxOutput(data[i]);
        n += tmp_vector[i].size();
    }
    std::vector<uint8_t> out_vector;
    out_vector.reserve(n);
    for (auto t : tmp_vector) out_vector.insert(out_vector.end(), t.begin(), t.end());
    return out_vector;
}

struct TxWitness {
    uint8_t stackItems;
    uint8_t size;
    std::vector<uint8_t> item;
};

std::vector<uint8_t> merge_TxWitness(const TxWitness& data) {
    std::vector<uint8_t> out_vector;
    out_vector.reserve(2 + data.size);
    out_vector.insert(out_vector.end(), data.stackItems);
    out_vector.insert(out_vector.end(), data.size);
    out_vector.insert(out_vector.end(), data.item.begin(), data.item.end());
    return out_vector;
}

struct Tx {
    std::vector<uint8_t> version; // le
    uint8_t marker;
    uint8_t flag;
    uint8_t input_count;
    std::vector<TxInput> inputs;
    uint8_t output_count;
    std::vector<TxOutput> outputs;
    std::vector<TxWitness> witnesses;
    std::vector<uint8_t> locktime;
};

std::vector<uint8_t> merge_Tx(const Tx& data, bool witness = false, bool forHash = false) {
    std::vector<uint8_t> dataInput = merge_TxInputs(data.inputs);
    std::vector<uint8_t> dataOutput = merge_TxOutputs(data.outputs);
    std::vector<uint8_t> dataWitness = merge_TxWitness(data.witnesses[0]);
    size_t n = data.version.size() + 2 + dataInput.size() + dataOutput.size() + dataWitness.size();
    if (witness) n += 2;
    std::vector<uint8_t> out_vector;
    out_vector.reserve(n);
    out_vector.insert(out_vector.end(), data.version.begin(), data.version.end());
    if (witness) {
        out_vector.insert(out_vector.end(), data.marker);
        out_vector.insert(out_vector.end(), data.flag);
    }
    out_vector.insert(out_vector.end(), data.input_count);
    out_vector.insert(out_vector.end(), dataInput.begin(), dataInput.end());
    out_vector.insert(out_vector.end(), data.output_count);
    out_vector.insert(out_vector.end(), dataOutput.begin(), dataOutput.end());
    if (!forHash && witness) out_vector.insert(out_vector.end(), dataWitness.begin(), dataWitness.end());
    out_vector.insert(out_vector.end(), data.locktime.begin(), data.locktime.end());
    if (forHash && witness) out_vector.insert(out_vector.end(), dataWitness.begin(), dataWitness.end());
    return out_vector;
}

uint8_t get_size_vecror2(const std::vector<std::vector<uint8_t>>& data) {
    size_t n = data.size();
    if (n == 1) n --;
    for (auto d : data) n += d.size();
    return (uint8_t)n;
}

std::vector<uint8_t> get_txid(const Tx& transaction) {
    std::vector<uint8_t> data = merge_Tx(transaction);
    return double_sha256(data);
}

std::vector<uint8_t> get_wtxid(const Tx& transaction) {
    std::vector<uint8_t> data = merge_Tx(transaction, true, true);
    return double_sha256(data);
}