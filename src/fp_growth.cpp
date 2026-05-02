#include "fp_growth.hpp"
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <sstream>

namespace {
void print_json_string(std::ostream &out, const std::string &value) {
    out << '"';
    for (char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
        }
    }
    out << '"';
}

void print_json_array(std::ostream &out, const Itemset &items) {
    out << '[';
    bool first = true;
    for (const auto &item : items) {
        if (!first) out << ',';
        print_json_string(out, item);
        first = false;
    }
    out << ']';
}

} // namespace

void FPGrowth::count_items(std::unordered_map<std::string, int> &item_counts) {
    const int max_threads = omp_get_max_threads();
    std::vector<std::unordered_map<std::string, int>> local_counts(max_threads);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        auto &local = local_counts[tid];

        #pragma omp for schedule(static)
        for (long long i = 0; i < static_cast<long long>(transactions.size()); ++i) {
            for (const auto &item : transactions[i]) {
                ++local[item];
            }
        }
    }

    for (auto &local : local_counts) {
        for (auto &[item, count] : local) {
            item_counts[item] += count;
        }
    }
}

void FPGrowth::build_tree(const Transaction &sorted_items,
                          const std::unordered_map<std::string, int> &item_counts) {
    header_table.clear();
    header_table.reserve(item_counts.size());

    for (const auto &[item, count] : item_counts) {
        if (count >= min_support_count) {
            HeaderEntry entry;
            entry.count = count;
            header_table.emplace(item, entry);
        }
    }

    std::unordered_map<std::string, int> rank;
    rank.reserve(sorted_items.size());
    for (size_t i = 0; i < sorted_items.size(); ++i) {
        rank[sorted_items[i]] = static_cast<int>(i);
    }

    // Ten etap jest niezależny dla każdej transakcji, więc OpenMP ma tu sens.
    Transactions prepared(transactions.size());

    #pragma omp parallel for schedule(dynamic, 256)
    for (long long i = 0; i < static_cast<long long>(transactions.size()); ++i) {
        const auto &trans = transactions[i];
        Transaction filtered;
        filtered.reserve(trans.size());

        for (const auto &item : trans) {
            if (header_table.find(item) != header_table.end()) {
                filtered.push_back(item);
            }
        }

        std::sort(filtered.begin(), filtered.end(), [&](const auto &a, const auto &b) {
            return rank.find(a)->second < rank.find(b)->second;
        });

        prepared[i] = std::move(filtered);
    }

    // Wstawianie do jednego FP-tree zostaje sekwencyjne, bo modyfikuje wspólne drzewo.
    for (const auto &filtered : prepared) {
        if (!filtered.empty()) {
            insert_tree(filtered, root, header_table);
        }
    }
}

void FPGrowth::insert_tree(const Transaction &items, NodePointer node,
                           HeaderTable &table, size_t idx) {
    // Iteracyjnie zamiast rekurencji: mniej narzutu dla długich transakcji.
    for (size_t pos = idx; pos < items.size(); ++pos) {
        const std::string &item = items[pos];
        NodePointer child;

        auto child_it = node->children.find(item);
        if (child_it != node->children.end()) {
            child = child_it->second;
            child->count += 1;
        } else {
            child = std::make_shared<Node>(item, 1, node);
            node->children.emplace(item, child);

            auto &entry = table[item];
            if (!entry.head) {
                entry.head = child;
                entry.tail = child;
            } else {
                entry.tail->next_link = child;
                entry.tail = child;
            }
        }

        node = child;
    }
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
                cond_patterns.emplace_back(std::move(path), node->count);
            }

            node = node->next_link.lock();
        }

        std::unordered_map<std::string, int> cond_counts;
        for (auto &[path, c] : cond_patterns) {
            for (auto &x : path) {
                cond_counts[x] += c;
            }
        }

        HeaderTable cond_table;
        cond_table.reserve(cond_counts.size());
        for (auto &[k, v] : cond_counts) {
            if (v >= min_support_count) {
                HeaderEntry e;
                e.count = v;
                cond_table.emplace(k, e);
            }
        }

        if (!cond_table.empty()) {
            mine_tree(cond_table, new_prefix);
        }
    }
}

void FPGrowth::generate_rules() {
    std::vector<std::pair<Itemset, int>> itemsets;
    itemsets.reserve(frequent_itemsets.size());
    for (const auto &[itemset, count] : frequent_itemsets) {
        if (itemset.size() >= 2) {
            itemsets.emplace_back(itemset, count);
        }
    }

    // Wypisywanie jest często wąskim gardłem. Bufor lokalny ogranicza walkę o stdout.
    #pragma omp parallel
    {
        std::ostringstream local_out;

        #pragma omp for schedule(dynamic)
        for (long long idx = 0; idx < static_cast<long long>(itemsets.size()); ++idx) {
            const auto &itemset = itemsets[static_cast<size_t>(idx)].first;
            int count = itemsets[static_cast<size_t>(idx)].second;

            Transaction items(itemset.begin(), itemset.end());
            int n = static_cast<int>(items.size());
            if (n >= 31) continue; // zabezpieczenie przed overflow maski int

            for (int mask = 1; mask < (1 << n) - 1; ++mask) {
                Itemset A, B;

                for (int i = 0; i < n; ++i) {
                    if (mask & (1 << i)) A.insert(items[i]);
                    else B.insert(items[i]);
                }

                auto antecedent_it = frequent_itemsets.find(A);
                if (antecedent_it == frequent_itemsets.end()) continue;

                double conf = static_cast<double>(count) / antecedent_it->second;
                double supp = static_cast<double>(count) / transactions.size();

                if (conf >= min_confidence) {
                    local_out << "{\"A\":";
                    print_json_array(local_out, A);
                    local_out << ",\"B\":";
                    print_json_array(local_out, B);
                    local_out << ",\"supp\":" << supp;
                    local_out << ",\"conf\":" << conf;
                    local_out << "}\n";
                }
            }
        }

        #pragma omp critical
        {
            std::cout << local_out.str();
        }
    }
}

void FPGrowth::solve() {
    std::unordered_map<std::string, int> item_counts;
    count_items(item_counts);

    Transaction sorted_items;
    sorted_items.reserve(item_counts.size());
    for (auto &[item, _] : item_counts) {
        sorted_items.push_back(item);
    }

    std::sort(sorted_items.begin(), sorted_items.end(), [&](const auto &a, const auto &b) {
        return item_counts[a] > item_counts[b];
    });

    build_tree(sorted_items, item_counts);

    mine_tree(header_table, {});

    generate_rules();
}
