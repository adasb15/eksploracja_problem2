#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using Itemset = std::set<std::string>;

using FrequentMap = std::map<Itemset, int>;

using Transaction = std::vector<std::string>;

using Transactions = std::vector<Transaction>;

class Node {
  public:
    std::string item;
    int count;

    std::weak_ptr<Node> parent;
    std::unordered_map<std::string, std::shared_ptr<Node>> children;
    std::weak_ptr<Node> next_link;

    Node(const std::string &item, int count,
         std::shared_ptr<Node> parent = nullptr)
        : item(item), count(count), parent(parent) {}
};

using NodePointer = std::shared_ptr<Node>;

class HeaderEntry {
  public:
    int count;
    NodePointer head;
};

using HeaderTable = std::unordered_map<std::string, HeaderEntry>;

class Rule {
  public:
    Transaction A;
    Transaction B;
    double supp;
    double conf;
};