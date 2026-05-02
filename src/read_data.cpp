#include "read_data.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

Transactions read_data(const std::string &datapath) {
    std::ifstream file(datapath);
    Transactions raw_transactions;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << datapath << "\n";
        return raw_transactions;
    }

    std::string line;
    std::getline(file, line); // skip header

    std::unordered_map<std::string, std::unordered_set<std::string>> trans_dict;

    while (std::getline(file, line)) {
        size_t first_comma = line.find(',');
        if (first_comma == std::string::npos) {
            continue;
        }

        std::string id = line.substr(0, first_comma);
        if (id.empty() || !std::all_of(id.begin(), id.end(), [](unsigned char ch) { return std::isdigit(ch); })) {
            continue;
        }

        size_t second_comma = line.find(',', first_comma + 1);
        std::string item = line.substr(
            first_comma + 1,
            second_comma == std::string::npos ? std::string::npos : second_comma - first_comma - 1
        );

        if (!item.empty()) {
            trans_dict[id].insert(std::move(item));
        }
    }

    for (auto &[_, items] : trans_dict) {
        raw_transactions.emplace_back(items.begin(), items.end());
    }

    return raw_transactions;
}
