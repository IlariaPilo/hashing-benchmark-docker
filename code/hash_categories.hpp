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

// Define a helper type trait to check if 'train' member function exists
template <typename T>
struct has_train_member {
private:
    template <typename C>
    static constexpr auto check(C*) -> typename std::is_same<decltype(std::declval<C>().train(std::declval<typename C::iterator>(), std::declval<typename C::iterator>(), 0)), void>::type;

    template <typename>
    static constexpr std::false_type check(...);

public:
    static constexpr bool value = std::is_same<decltype(check<T>(nullptr)), std::true_type>::value;
};

#endif