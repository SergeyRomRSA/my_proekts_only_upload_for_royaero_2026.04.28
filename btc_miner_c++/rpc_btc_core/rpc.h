#include <iostream>     // Для вывода на экран (std::cout) и ввода (std::cin)
#include <string>       // Для работы со строками std::string
#include <curl/curl.h>  // Библиотека libcurl для HTTP-запросов
#include "../json.hpp"     // Библиотека nlohmann/json для работы с JSON

using json = nlohmann::json; // Создаём удобное сокращение типа json

// Данные для подключения к Bitcoin Core RPC
const std::string rpcUser = "user";      // RPC user из bitcoin.conf
const std::string rpcPassword = "password";  // RPC password из bitcoin.conf
const std::string url_btc = "http://127.0.0.1:8332/"; // Адрес RPC-сервера
const std::string wallet_name = "wallet_name";
const std::string url_wallet = url_btc + "wallet/" + wallet_name;

// Функция обратного вызова для libcurl
// Вызывается каждый раз, когда приходят данные от сервера
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    // userp — указатель на std::string, куда будем добавлять данные
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb; // возвращаем количество обработанных байт
}

json request(const std::string& requestData, const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("curl init failed");
    }

    std::string readBuffer;
    std::string auth = rpcUser + ":" + rpcPassword;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Настраиваем curl
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());           // URL сервера
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth.c_str());      // Авторизация
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str()); // Данные POST
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);        // Заголовки
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);    // Функция обратного вызова
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);          // Буфер для записи ответа


    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        throw std::runtime_error("curl_easy_perform() failed");
    }

    json response = json::parse(readBuffer);

    if (!response["error"].is_null()) {
        std::cerr << "Request Error: " << requestData << std::endl;
        std::cerr << "RPC Error: " << response["error"] << std::endl;
        // throw std::runtime_error("RPC Error");
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return response["result"];
}


json getblocktemplate(){
    // Создаём JSON-RPC запрос
    json rpcRequest = {
        {"jsonrpc", "1.0"},                // Версия протокола JSON-RPC
        {"id", "curltest"},                // Идентификатор запроса
        {"method", "getblocktemplate"},    // Метод RPC, который вызываем
        {"params", { { {"rules", {"segwit"}} } }} // Параметры метода (rules=["segwit"])
    };
    return request(rpcRequest.dump(), url_btc);
}

int getblockcount() {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getblockcount"},
        {"method", "getblockcount"},
        {"params", json::array()}
    };

    return request(rpcRequest.dump(), url_btc).get<int>();
}

std::string getbestblockhash() {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getbestblockhash"},
        {"method", "getbestblockhash"},
        {"params", json::array()}
    };

    return request(rpcRequest.dump(), url_btc).get<std::string>();
}

json getblock(const std::string& blockhash, int verbosity = 2) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getblock"},
        {"method", "getblock"},
        {"params", { blockhash, verbosity }}
    };

    return request(rpcRequest.dump(), url_btc);
}

json getrawmempool(bool verbose = false) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getrawmempool"},
        {"method", "getrawmempool"},
        {"params", { verbose }}
    };

    return request(rpcRequest.dump(), url_btc);
}

json getmempoolinfo() {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getmempoolinfo"},
        {"method", "getmempoolinfo"},
        {"params", json::array()}
    };

    return request(rpcRequest.dump(), url_btc);
}

json getmempoolentry(const std::string& txid) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getmempoolentry"},
        {"method", "getmempoolentry"},
        {"params", { txid }}
    };
    return request(rpcRequest.dump(), url_btc);
}


json submitblock(const std::string& blockHex) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "submitblock"},
        {"method", "submitblock"},
        {"params", { blockHex }}
    };

    return request(rpcRequest.dump(), url_btc);
}

json getrawtransaction(const std::string& txid, bool verbose = true) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getrawtransaction"},
        {"method", "getrawtransaction"},
        {"params", { txid, verbose }}
    };
    return request(rpcRequest.dump(), url_btc);
}

json getnewaddress(const std::string& label = "miner",
                   const std::string& type = "bech32") {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "getnewaddress"},
        {"method", "getnewaddress"},
        {"params", {label, type}}
    };
    return request(rpcRequest.dump(), url_wallet);
}

json validateaddress(const std::string& address) {
    json rpcRequest = {
        {"jsonrpc", "1.0"},
        {"id", "validateaddress"},
        {"method", "validateaddress"},
        {"params", {address}}
    };
    return request(rpcRequest.dump(), url_wallet);
}

