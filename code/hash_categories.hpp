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
    {"MultiplicationHash", HashCategories::CLASSIC},
    {"AquaHash", HashCategories::CLASSIC},
    {"XXHash3", HashCategories::CLASSIC},
    {"MWHC", HashCategories::PERFECT},
    {"BitMWHC", HashCategories::PERFECT},
    {"RecSplit", HashCategories::PERFECT}
};

HashCategories get_category(std::string full_name) {
    for (const auto& pair : HASH_FN_TYPES) {
        std::string name = pair.first;
        size_t pos = full_name.find(name);
        if (pos != std::string::npos) {
            return pair.second;
        }
    }
    // nothing was found
    std::cout << full_name << std::endl;
    return HashCategories::UNKNOWN;
}


// Function to check if a type is "learned"
// To be called as : if (is_learned<RMIHash>()) ...
template <typename T>
bool is_learned() {
    std::string full_name = typeid(T).name();
    HashCategories type = get_category(full_name);
    return (type == HashCategories::LEARNED);
}
template <typename T>
bool is_classic() {
    std::string full_name = typeid(T).name();
    HashCategories type = get_category(full_name);
    return (type == HashCategories::CLASSIC);
}
template <typename T>
bool is_perfect() {
    std::string full_name = typeid(T).name();
    HashCategories type = get_category(full_name);
    return (type == HashCategories::PERFECT);
}
template <typename T>
HashCategories get_fn_type() {
    std::string full_name = typeid(T).name();
    return get_category(full_name);
}

// Define a helper type trait to check if 'train' member function exists
template <typename T, typename = void> struct has_train_sfinae : std::false_type {};
template <typename T> struct has_train_sfinae<T,
    decltype(void(std::declval<T &>().train(std::declval<const typename T::value_type &>())))
> : std::true_type {};
template <typename T> inline constexpr bool has_train = has_train_sfinae<T>::value;

// Define a helper type trait to check if 'construct' member function exists
template <typename T, typename = void> struct has_construct_sfinae : std::false_type {};
template <typename T> struct has_construct_sfinae<T,
    decltype(void(std::declval<T &>().construct(std::declval<const typename T::value_type &>())))
> : std::true_type {};
template <typename T> inline constexpr bool has_construct = has_construct_sfinae<T>::value;

#endif
