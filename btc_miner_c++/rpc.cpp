#include <iostream>     // Для вывода на экран (std::cout) и ввода (std::cin)
#include <string>       // Для работы со строками std::string
#include <curl/curl.h>  // Библиотека libcurl для HTTP-запросов
#include "json.hpp"     // Библиотека nlohmann/json для работы с JSON
#include <chrono>
#include <sstream>
#include "rpc_btc_core/rpc.h"
#include "txc/header_form.h"
// #include "miner/double_sha256_mining.h"

// g++ rpc.cpp -o rpc -lcurl -std=c++17 -lcrypto -mavx2 -mfma -O3


using json = nlohmann::json; // Создаём удобное сокращение типа json

// void test_v256() {
//     v256 a = _mm256_setzero_si256();
//     v256 b = _mm256_set1_epi32(1);
    
//     for (int i = 0; i < 16; i ++) {
//         std::cout << ((uint32_t*)&a)[1] << std::endl;
//         std::cout << ((uint32_t*)&a)[5] << std::endl;
//         a = ADD(a, b);
//     }
// }

int main() {
    // test_v256();
    // int l = 0;
    // uint32_t delta = 0;
    // statistic[1] = 1;
    while (true) {
        std::cout << ";-------------------" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        json result = getblocktemplate();
        json miner_address = getnewaddress();
        std::string address = miner_address.get<std::string>();
        json addr_info = validateaddress(address);

        // std::cout << result;
        // -------------
        // Работа с транзакциями
        size_t txCount = result["transactions"].size();
        int64_t totalFees = 0;
        for (auto& tx : result["transactions"]) {
            if (tx.contains("fee"))
                totalFees += tx["fee"].get<int64_t>();
        }
        
        std::vector<uint8_t> scriptPubKeyHex = hex_to_u8(addr_info["scriptPubKey"]);
        std::vector<uint8_t> witnessHex = hex_to_u8("6a24aa21a9ede2f61c3f71d1defd3fa999dfa36953755c690689799962b48bebd836974e8cf9");
        uint32_t ver = result["version"].get<uint32_t>();
        // if (statistic[1] == 0) {
        //     ver += delta;
        //     delta += 3328;
        // }
        
        // -----------------
        
        std::vector<std::vector<uint8_t>> scriptSigCoinbase = {
            reverse_u8(i32_to_u8(result["height"])),
            string_to_u8("RSA")
            // i32_to_u8(780637500)
        };
        
        TxInput icb1 = {
            create_u8(32),
            create_u8(4, 0xff),
            get_size_vecror2(scriptSigCoinbase),
            scriptSigCoinbase,
            create_u8(4, 0xff)
        };
        std::vector<TxInput> inputCoinbase = {icb1};
        
        TxOutput ocb1 = {
            reverse_u8(i64_to_u8(result["coinbasevalue"].get<int64_t>() - totalFees)),
            (uint8_t)scriptPubKeyHex.size(),
            scriptPubKeyHex
        };
        
        TxOutput ocb2 = {
            create_u8(8),
            (uint8_t)witnessHex.size(),
            witnessHex
        };
        std::vector<TxOutput> outputCoinbase = {ocb1, ocb2};

        TxWitness wcb1 = {
            0x01,
            0x20,
            create_u8(32)
        };
        std::vector<TxWitness> witnessCoinbase = {wcb1};

        Tx coinbase {
            reverse_u8(i32_to_u8(1)),
            0x00,
            0x01,
            0x01,
            inputCoinbase,
            0x02,
            outputCoinbase,
            witnessCoinbase,
            create_u8(4)
        };

        // -----------------

        HeaderBlock info{
            reverse_u8(i32_to_u8(ver)),
            reverse_u8(hex_to_u8(result["previousblockhash"])),
            get_txid(coinbase),
            reverse_u8(i32_to_u8(result["curtime"].get<uint32_t>())),
            reverse_u8(i32_to_u8(std::stoul(result["bits"].get<std::string>(), nullptr, 16))),
            create_u8(4)
        };

        set_Version(info.version);
        set_prevBlock(u8_to_u32(info.prevBlock));
        set_MercleRoot(u8_to_u32(info.mercleRoot));
        set_Time(info.time);
        set_Bits(info.bits);

        std::cout << "Height        | " << result["height"] << std::endl;
        // std::cout << "Version       | " << u8_to_hex(reverse_u8(info.version)) << std::endl;
        std::cout << "PrevBlockHash | " << u8_to_hex(reverse_u8(info.prevBlock)) << std::endl;
        std::cout << "MerkleRoot    | " << u8_to_hex(info.mercleRoot) << std::endl;
        std::cout << "CurrentTime   | " << u8_to_hex(reverse_u8(info.time)) << std::endl;
        // std::cout << "Bits          | " << u8_to_hex(reverse_u8(info.bits)) << std::endl;
        // std::cout << std::endl;
        // std::cout << "Subsidy       | " << (result["coinbasevalue"].get<int64_t>() - totalFees) << std::endl;
        std::cout << "CointTX       | " << txCount << std::endl;
        std::cout << "Address       | " << address << std::endl;
        std::cout << "scriptPubKey  | " << u8_to_hex(ocb1.scriptPubKey) << std::endl;
        std::cout << "Block (hash)  | " << u8_to_hex(double_sha256(get_info(info))) << std::endl;
        // std::cout << std::endl;
        // std::cout << "Block Header (hex): " << u8_to_hex(get_info(info)) << std::endl;
        // std::cout << std::endl;
        // std::cout << "Coinbase (hex): " << u8_to_hex(merge_Tx(coinbase, true)) << std::endl;
        // std::cout << std::endl;

        // nonce (будет меняться)
        statistic[0] = 0;statistic[1] = 0;statistic[2] = 0;statistic[3] = 0;
        uint32_t nonce = nonceSearch();
        // std::cout << "A             | " << statistic[0] << std::endl;
        // std::cout << "B             | " << statistic[1] << std::endl;
        // std::cout << "C             | " << statistic[2] << std::endl;
        // std::cout << std::endl;
        if (nonce_found.load()) {
            info.nonce = i32_to_u8(nonce);
            std::string block_hex = u8_to_hex(merge_vector(get_info(info), merge_Tx(coinbase, true)));

            std::cout << "Block (hash)  | " << u8_to_hex(double_sha256(get_info(info))) << std::endl;
            std::cout << std::endl;
            std::cout << "Block Header (hex): " << u8_to_hex(get_info(info)) << std::endl;
            std::cout << std::endl;


            // подготовка блока: заголовок + транзакции
            json response = submitblock(block_hex);
            std::cout << "submitblock response: " << response.dump(4) << std::endl;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        uint64_t minute = duration.count() / 60000;
        uint64_t second = (duration.count() - 60000 * minute) / 1000;
        std::string preffix;
        std::string suffix;
        if (minute < 10) preffix = "Время выполнения: 0";
        else preffix = "Время выполнения: ";
        if (second < 10) suffix = ":0";
        else suffix = ":";
        std::cout << preffix << minute << suffix << second << std::endl;
        std::cout << std::endl;
        // l = 1;
    }

    return 0; // Завершаем программу успешно
}


