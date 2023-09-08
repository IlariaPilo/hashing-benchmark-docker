#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <iostream>
#include <vector>
#include <unordered_map>

using namespace std;

/* 

------------------ configs.hpp ------------------
This file can be used to modify the benchmark parameters.
Parameters are separated in two groups:
- hash functions
- collision strategy specific parameters
Each benchmark will be executed
    for all hash functions
        for all collision strategies
            for all overalloc values

 */

namespace config {

    enum HashCategories {
        LEARNED,    // 0
        CLASSIC,    // 1
        PERFECT     // 2
    };

    enum CollisionCategories {
        LINEAR,     // 0
        CHAINED,    // 1
        CUCKOO      // 2
    };

    enum HashF {
        RMIHash, RadixSplineHash, PGMHash,
        MURMUR, MultPrime64, FibonacciPrime64, AquaHash, XXHash3,
        MWHC, BitMWHC, RecSplit
    };

    const unordered_map<HashF, HashCategories> AVAILABLE_HASH_FUNCTIONS = {
        // LEARNED
        {HashF::RMIHash, HashCategories::LEARNED},
        {HashF::RadixSplineHash, HashCategories::LEARNED},
        {HashF::PGMHash, HashCategories::LEARNED},

        // CLASSIC
        {HashF::MURMUR, HashCategories::CLASSIC},
        {HashF::MultPrime64, HashCategories::CLASSIC},
        {HashF::FibonacciPrime64, HashCategories::CLASSIC},
        {HashF::AquaHash, HashCategories::CLASSIC},
        {HashF::XXHash3, HashCategories::CLASSIC},

        // PERFECT
        {HashF::MWHC, HashCategories::PERFECT},
        {HashF::BitMWHC, HashCategories::PERFECT},
        {HashF::RecSplit, HashCategories::PERFECT}       // FIXME - not sure
    };


    // ========= START - modify these constants as you wish ========= //

    /* ------ HASH FUNCTIONS ------
    All the available hash functions are displayed in the AVAILABLE_HASH_FUNCTION dictionary.
    Each function is associated to its category (learned, classic or perfect). 
    */
    const vector<HashF> functions_config = {
        HashF::RMIHash, HashF::RadixSplineHash, HashF::PGMHash,
        HashF::MURMUR, HashF::MultPrime64, HashF::FibonacciPrime64, HashF::AquaHash, HashF::XXHash3,
        HashF::MWHC, HashF::BitMWHC, HashF::RecSplit
    };

    /* ------ COLLISION MANAGEMENT ------
    BUCKET_SIZE
    The number of elements which are allowed in each bucket [default 1].
        OVERALLOC
    TODO
        KICKING_BIAS
    Controls the kicking strategy in the Cuckoo collision management strategy.
    If set to 0, then the BALANCED strategy is used. 
    Otherwise, the BIASED strategy is used, and KICKING_BIAS represents the value of the bias.
    */
    const int DEFAULT_BUCKET_SIZE = 1;
    // [ only for learned functions ]
    const int DEFAULT_MAX_MODELS = 100;
    const int DEFAULT_MAX_ERRORS = 1024;
    const int num_radix_bits = 18;

    // LINEAR
    const int LINEAR_BUCKET_SIZE = DEFAULT_BUCKET_SIZE;
    const vector<int> LINEAR_OVERALLOC = {34, 54, 82, 122, 185, 300};
    // [ only for learned functions ]
    const int LINEAR_MAX_MODELS = DEFAULT_MAX_MODELS;
    const int LINEAR_MAX_ERRORS = DEFAULT_MAX_ERRORS;

    // CHAINED
    const int CHAINED_BUCKET_SIZE = DEFAULT_BUCKET_SIZE;
    const vector<int> CHAINED_OVERALLOC = {10050, 10067, 10080, 0, 34, 100, 300};
    // [ only for learned functions ]
    const int CHAINED_MAX_MODELS = DEFAULT_MAX_MODELS;
    const int CHAINED_MAX_ERRORS = DEFAULT_MAX_ERRORS;

    // CUCKOO
    const int CUCKOO_BUCKET_SIZE = 4;
    const vector<int> CUCKOO_OVERALLOC = {34, 25, 17, 11, 5};
    const int KICKING_BIAS = 5;
    // [ only for learned functions ]
    const int CUCKOO_MAX_MODELS = 100000;
    const int CUCKOO_MAX_ERRORS = 32;

    // ============================ END ============================ //

    // Define a struct to represent the configuration for each type
    struct ConfigBM {
        CollisionCategories collision_strategy;
        int bucket_size;
        vector<int> overalloc;
        int kicking_bias;
        int max_models;
        int max_errors;
    };

    const ConfigBM linear_config = {
        CollisionCategories::LINEAR,
        LINEAR_BUCKET_SIZE,
        LINEAR_OVERALLOC,
        0,
        LINEAR_MAX_MODELS,
        LINEAR_MAX_ERRORS
    };

    const ConfigBM chained_config = {
        CollisionCategories::CHAINED,
        CHAINED_BUCKET_SIZE,
        CHAINED_OVERALLOC,
        0,
        CHAINED_MAX_MODELS,
        CHAINED_MAX_ERRORS
    };
    
    const ConfigBM cuckoo_config = {
        CollisionCategories::CUCKOO,
        CUCKOO_BUCKET_SIZE,
        CUCKOO_OVERALLOC,
        KICKING_BIAS,
        CUCKOO_MAX_MODELS,
        CUCKOO_MAX_ERRORS
    };

}       // namespace config

#endif // __CONFIG_HPP__
