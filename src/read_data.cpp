#include "read_data.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
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
        std::stringstream ss(line);
        std::string id, item;

        if (std::getline(ss, id, ',') && std::getline(ss, item, ',')) {
            // check if id is numeric
            if (!id.empty() && std::all_of(id.begin(), id.end(), ::isdigit)) {
                trans_dict[id].insert(item);
            }
        }
    }

    for (auto &[_, items] : trans_dict) {
        raw_transactions.emplace_back(items.begin(), items.end());
    }

    return raw_transactions;
}
