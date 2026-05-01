#include "fp_growth.hpp"
#include <algorithm>
#include <iostream>

void FPGrowth::count_items(std::unordered_map<std::string, int> &item_counts) {
    for (const auto &t : transactions) {
        for (const auto &item : t) {
            item_counts[item]++;
        }
    }
}

void FPGrowth::build_tree(const Transaction &sorted_items) {
    header_table.clear();

    std::unordered_map<std::string, int> item_counts;
    count_items(item_counts);

    for (auto &[item, count] : item_counts) {
        if (count >= min_support_count) {
            header_table[item] = {count, nullptr};
        }
    }

    for (const auto &trans : transactions) {
        Transaction filtered;

        for (const auto &item : sorted_items) {
            if (std::find(trans.begin(), trans.end(), item) != trans.end()) {
                filtered.push_back(item);
            }
        }

        if (!filtered.empty()) {
            insert_tree(filtered, root, header_table);
        }
    }
}

void FPGrowth::insert_tree(const Transaction &items, NodePointer node,
                           HeaderTable &table, size_t idx) {

    if (idx >= items.size())
        return;

    const std::string &item = items[idx];
    NodePointer child;

    if (node->children.count(item)) {
        child = node->children[item];
        child->count += 1;
    } else {
        child = std::make_shared<Node>(item, 1, node);
        node->children[item] = child;

        auto &entry = table[item];
        if (!entry.head) {
            entry.head = child;
        } else {
            auto cur = entry.head;
            while (auto nxt = cur->next_link.lock())
                cur = nxt;
            cur->next_link = child;
        }
    }

    insert_tree(items, child, table, idx + 1);
}

void FPGrowth::mine_tree(HeaderTable &table, Itemset prefix) {
    for (auto &[item, entry] : table) {
        Itemset new_prefix = prefix;
        new_prefix.insert(item);

        frequent_itemsets[new_prefix] = entry.count;

        std::vector<std::pair<Transaction, int>> cond_patterns;

        auto node = entry.head;
        while (node) {
            Transaction path;
            auto p = node->parent.lock();

            while (p && !p->item.empty()) {
                path.push_back(p->item);
                p = p->parent.lock();
            }

            if (!path.empty()) {
                cond_patterns.emplace_back(path, node->count);
            }

            node = node->next_link.lock();
        }

        std::unordered_map<std::string, int> cond_counts;
        for (auto &[path, c] : cond_patterns) {
            for (auto &x : path)
                cond_counts[x] += c;
        }

        HeaderTable cond_table;
        for (auto &[k, v] : cond_counts) {
            if (v >= min_support_count) {
                cond_table[k] = {v, nullptr};
            }
        }

        if (!cond_table.empty()) {
            mine_tree(cond_table, new_prefix);
        }
    }
}

void FPGrowth::generate_rules() {
    bool found = false;

    for (auto &[itemset, count] : frequent_itemsets) {
        if (itemset.size() < 2)
            continue;

        Transaction items(itemset.begin(), itemset.end());
        int n = items.size();

        for (int mask = 1; mask < (1 << n) - 1; ++mask) {
            Itemset A, B;

            for (int i = 0; i < n; ++i) {
                if (mask & (1 << i))
                    A.insert(items[i]);
                else
                    B.insert(items[i]);
            }

            auto it = frequent_itemsets.find(A);
            if (it == frequent_itemsets.end())
                continue;

            double conf = (double)count / it->second;
            double supp = (double)count / transactions.size();

            // output a json-like format
            if (conf >= min_confidence) {
                std::cout << "{'A':";
                for (auto &a : A)
                    std::cout << a << " ";
                std::cout << "] => [";
                for (auto &b : B)
                    std::cout << b << " ";
                std::cout << "]\n";
            }
        }
    }
}

void FPGrowth::solve() {
    std::unordered_map<std::string, int> item_counts;
    count_items(item_counts);

    Transaction sorted_items;
    for (auto &[item, _] : item_counts)
        sorted_items.push_back(item);

    std::sort(sorted_items.begin(), sorted_items.end(),
              [&](const auto &a, const auto &b) {
                  return item_counts[a] > item_counts[b];
              });

    build_tree(sorted_items);

    mine_tree(header_table, {});

    generate_rules();
}