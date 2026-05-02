#pragma once

#include "fp_types.hpp"
#include <cmath>

class FPGrowth {
  private:
    bool verbose;

    double min_support;
    double min_confidence;

    Transactions transactions;
    size_t n_trans;
    int min_support_count;

    NodePointer root;
    HeaderTable header_table;
    FrequentMap frequent_itemsets;

  public:
    FPGrowth(double min_support, double min_confidence, Transactions data,
             bool verbose) {

        this->verbose = verbose;
        this->min_support = min_support;
        this->min_confidence = min_confidence;
        this->transactions = std::move(data);
        this->n_trans = transactions.size();
        this->min_support_count = static_cast<int>(std::ceil(min_support * n_trans));
        this->root = std::make_shared<Node>("", 0);
    }

    void solve();

  private:
    void count_items(std::unordered_map<std::string, int> &item_counts);

    void build_tree(const Transaction &sorted_items);

    void mine_tree(HeaderTable &table, Itemset prefix);

    void insert_tree(const Transaction &items, NodePointer node,
                     HeaderTable &table, size_t idx = 0);

    void generate_rules();
};
