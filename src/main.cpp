#include "fp_growth.hpp"
#include "read_data.hpp"
#include <iostream>

// TODO: add verbose handling
// TODO: replace stdout data transmission with something like: pybind11
// to fix the bizarelly poor performance (hopefully)

int main(int argc, char *argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    double min_support = std::stod(argv[1]);
    double min_confidence = std::stod(argv[2]);
    Transactions data = read_data(argv[3]);
    bool verbose = std::stoi(argv[4]);

    FPGrowth fp(min_support, min_confidence, data, verbose);

    fp.solve();
}
