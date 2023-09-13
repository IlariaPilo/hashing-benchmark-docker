// Add definition for the tree types of functions
#ifndef __HASH_CAT__
#define __HASH_CAT__

#include <unordered_map>
#include <string>

enum class HashCategories {
    LEARNED,
    CLASSIC,
    PERFECT,
    UNKNOWN
};

const std::unordered_map<std::string, HashCategories> HASH_FN_TYPES = {
    {"RMIHash", HashCategories::LEARNED},
    {"RadixSplineHash", HashCategories::LEARNED},
    {"PGMHash", HashCategories::LEARNED},
    {"MurmurFinalizer", HashCategories::CLASSIC},
    {"MultPrime64", HashCategories::CLASSIC},
    {"FibonacciPrime64", HashCategories::CLASSIC},
    {"AquaHash", HashCategories::CLASSIC},
    {"XXHash3", HashCategories::CLASSIC},
    {"MWHC", HashCategories::PERFECT},
    {"BitMWHC", HashCategories::PERFECT},
    {"RecSplit", HashCategories::PERFECT}
};

// Function to check if a type is "learned"
// To be called as : if (is_learned<RMIHash>()) ...
template <typename T>
bool is_learned() {
    std::string name = typeid(T).name();
    auto it = HASH_FN_TYPES.find(name);
    return (it != HASH_FN_TYPES.end() && it->second == HashCategories::LEARNED);
}
template <typename T>
bool is_classic() {
    std::string name = typeid(T).name();
    auto it = HASH_FN_TYPES.find(name);
    return (it != HASH_FN_TYPES.end() && it->second == HashCategories::CLASSIC);
}
template <typename T>
bool is_perfect() {
    std::string name = typeid(T).name();
    auto it = HASH_FN_TYPES.find(name);
    return (it != HASH_FN_TYPES.end() && it->second == HashCategories::PERFECT);
}
template <typename T>
HashCategories get_fn_type() {
    std::string name = typeid(T).name();
    auto it = HASH_FN_TYPES.find(name);
    if (it == HASH_FN_TYPES.end())
        return HashCategories::UNKNOWN;
    return it->second;
}

#endif