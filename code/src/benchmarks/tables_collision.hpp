#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <hashing.hpp>
#include <hashtable.hpp>
#include <iostream>
#include <iterator>
#include <learned_hashing.hpp>
#include <limits>
#include <masters_thesis.hpp>
#include <ostream>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <string>

#include <stdio.h>
#include <stdlib.h>

#include "../../thirdparty/perfevent/PerfEvent.hpp"
#include "../support/datasets.hpp"
#include "../support/probing_set.hpp"
#include "include/convenience/builtins.hpp"
#include "include/mmphf/rank_hash.hpp"
#include "include/rmi.hpp"

namespace _ {
  using Key = std::uint64_t;
  using Payload = std::uint64_t;

  const std::vector<std::int64_t> probe_distributions{
      static_cast<std::underlying_type_t<dataset::ProbingDistribution>>(
          dataset::ProbingDistribution::UNIFORM)};

  const std::vector<std::int64_t> dataset_sizes{100000000};
  // const std::vector<std::int64_t> succ_probability{100};
  // const std::vector<std::int64_t> point_query_prop{0,10,20,30,40,50,60,70,80,90,100};
  const std::vector<std::int64_t> datasets{
      static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::WIKI),
      static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::GAPPED_10),
      static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::UNIFORM),
      static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::NORMAL),
      // static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::SEQUENTIAL),
      // static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::OSM),
      static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::FB)};

  std::string previous_signature = "";

  void *prev_fn = nullptr;
  std::function<void()> free_fn = []() {};
  std::chrono::duration<double> tot_time(0);
  size_t collisions_count;
  size_t NOT_collisions_count;
  size_t num_elements;

  template <class HashFn, size_t RangeSize>
  static void CollisionStats(benchmark::State &state) {
    // Extract variables
    const auto dataset_size = static_cast<size_t>(state.range(0));
    const auto did = static_cast<dataset::ID>(state.range(1));
    const auto probing_dist =
        static_cast<dataset::ProbingDistribution>(state.range(2));
    // const auto succ_probability = static_cast<size_t>(state.range(3));
    const auto extra = static_cast<size_t>(state.range(3));

    std::string signature =
        std::string(typeid(HashFn).name()) + "_" + std::to_string(RangeSize) +
        "_" + std::to_string(dataset_size) + "_" + dataset::name(did) + "_" +
        dataset::name(probing_dist) + "_" + std::to_string(extra);
    if (previous_signature != signature) {
      #if PRINT
      std::cout << "performing setup... ";
      #endif
      auto start = std::chrono::steady_clock::now();

      std::vector<Key> keys = dataset::load_cached<Key>(did, dataset_size);
      #if PRINT
      std::cout << std::endl
                << "Key Size: " << keys.size() << std::endl << std::endl;
      #endif
      num_elements = keys.size();

      // ensure keys are sorted
      std::sort(keys.begin(), keys.end(),
              [](const auto& a, const auto& b) { return a < b; });

      if (keys.empty()) {
        // otherwise google benchmark produces an error ;(
        for (auto _ : state) {
        }
        std::cerr << "failed" << std::endl;
        return;
      }
      // at this point, we have ------ std::vector<std::pair<Key, Payload>> data

      // build function
      if (prev_fn != nullptr)
        free_fn();
      
      prev_fn = new HashFn();
      free_fn = []()
      { delete ((HashFn *)prev_fn); };

      HashFn &fn = *static_cast<HashFn *>(prev_fn);

      #if PRINT
      std::cout << "has_train<HashFn>: " << has_train_method<HashFn>::value << std::endl;
      std::cout << "has_construct<HashFn>: " << has_construct_method<HashFn>::value << std::endl;
      #endif
      // LEARNED FN
      if constexpr (has_train_method<HashFn>::value) {
        #if PRINT
        std::cout << "Learned function training starting...";
        #endif
        // train model on sorted data
        fn.train(keys.begin(), keys.end(), num_elements);
        #if PRINT
        std::cout << " done." << std::endl;
        #endif
      }
      // PERFECT FN
      if constexpr (has_construct_method<HashFn>::value) {
        #if PRINT
        std::cout << "Perfect function construction starting...";
        #endif
        // construct perfect hash table
        fn.construct(keys.begin(), keys.end());
        #if PRINT
        std::cout << " done." << std::endl;
        #endif
      }

      // measure time elapsed
      const auto end = std::chrono::steady_clock::now();
      std::chrono::duration<double> diff = end - start;
      #if PRINT
      std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
                << std::endl;
      #endif

      // ********************************************************************** //
      // logic to count collisions - maybe move in the loop

      std::vector<size_t> hash_v;
      hash_v.resize(num_elements, 0);

      HashCategories type = get_fn_type<HashFn>();

      if (type == HashCategories::UNKNOWN) {
        // TODO maybe remove this for? it depends on the position
        for (auto _ : state) {
        }
        std::cerr << "Type is unknown. Failed :(" << std::endl;
        return;
      }
      size_t index;
      std::chrono::time_point<std::chrono::steady_clock> _start_, _end_;
      tot_time = std::chrono::duration<double>(0); // Reset tot_time to zero

      int _c_ = 0;
      for (auto key : keys) {
        switch (type) {
          case HashCategories::PERFECT:
          case HashCategories::CLASSIC:
            _start_ = std::chrono::steady_clock::now();
            index = fn(key) % num_elements;
            _end_ = std::chrono::steady_clock::now();
            break;
          case HashCategories::LEARNED:
            _start_ = std::chrono::steady_clock::now();
            index = fn(key); // /dataset_size;
            _end_ = std::chrono::steady_clock::now();
            if (index >= num_elements) {
                // Throw a runtime exception
                throw std::runtime_error("Index is out of boundaries ("+std::to_string(index)+")");
            }
            break;
          // to remove the warning
          case HashCategories::UNKNOWN:
            break;
        }
        hash_v[index]++;
        tot_time += _end_ - _start_;
        _c_++;
      }
      if (_c_ != num_elements) {
          // Throw a runtime exception
          throw std::runtime_error("Something is wrong with the keys...");
      }
      collisions_count = 0;
      NOT_collisions_count = 0;
      // count collisions
      for (size_t i = 0; i < num_elements; i++) {
        if (hash_v[i] > 1)
          collisions_count += hash_v[i];
        else NOT_collisions_count += hash_v[i];
      }
      // ********************************************************************** //
    }

    // assert(prev_fn != nullptr);
    // HashFn* fn = (HashFn*)prev_fn;

    #if PRINT
    if (previous_signature != signature) {
      std::cout << std::endl
                << " Dataset Size: " << std::to_string(num_elements) << " Dataset: " << dataset::name(did) << std::endl;
    }
    #endif

    previous_signature = signature;

    size_t i = 0;
    for (auto _ : state) {}

    if ((collisions_count+NOT_collisions_count) != num_elements) {
        // Throw a runtime exception
        throw std::runtime_error("Collision number does not make sense! :c\nCollisions: "+std::to_string(collisions_count)+"\nNOT collisions: "+std::to_string(NOT_collisions_count));
    }

    state.counters["data_elem_count"] = num_elements;
    state.counters["tot_time_s"] = tot_time.count();
    state.counters["collisions"] = collisions_count;
    state.counters["dataset_id"] = static_cast<int>(did);
    state.counters["extra"] = extra;

    // std::stringstream ss;
    // ss << succ_probability;
    // std::string temp = ss.str();
    state.SetLabel("Collisions:" + std::string(typeid(HashFn).name()) + ":" + dataset::name(did) + ":" +
                   dataset::name(probing_dist) + ":" + std::to_string(extra));
  }

  using namespace masters_thesis;

#define CollisionBM(HashFn, extra)              \
  BENCHMARK_TEMPLATE(CollisionStats, HashFn, 0) \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions, {extra}});

  // Function aliases definition
  using RMIHash_10 = learned_hashing::RMIHash<std::uint64_t, 10>;
  using RMIHash_100 = learned_hashing::RMIHash<std::uint64_t, 100>;
  using RMIHash_1k = learned_hashing::RMIHash<std::uint64_t, 1000>;
  using RMIHash_10k = learned_hashing::RMIHash<std::uint64_t, 10000>;
  using RMIHash_100k = learned_hashing::RMIHash<std::uint64_t, 100000>;
  using RMIHash_1M = learned_hashing::RMIHash<std::uint64_t, 1000000>;
  using RMIHash_10M = learned_hashing::RMIHash<std::uint64_t, 10000000>;
  using RMIHash_100M = learned_hashing::RMIHash<std::uint64_t, 100000000>;

  using RadixSplineHash_4 = learned_hashing::RadixSplineHash<std::uint64_t, 18, 4>;
  using RadixSplineHash_16 = learned_hashing::RadixSplineHash<std::uint64_t, 18, 16>;
  using RadixSplineHash_128 = learned_hashing::RadixSplineHash<std::uint64_t, 18, 128>;
  using RadixSplineHash_1k = learned_hashing::RadixSplineHash<std::uint64_t, 18, 1024>;
  using RadixSplineHash_100k = learned_hashing::RadixSplineHash<std::uint64_t, 18, 100000>;

  using PGMHash_100k = learned_hashing::PGMHash<std::uint64_t, 100000, 100000, 500000000, float>;
  using PGMHash_1k = learned_hashing::PGMHash<std::uint64_t, 1024, 1024, 500000000, float>;
  using PGMHash_100 = learned_hashing::PGMHash<std::uint64_t, 128, 128, 500000000, float>;
  using PGMHash_32 = learned_hashing::PGMHash<std::uint64_t, 32, 32, 500000000, float>;
  using PGMHash_2 = learned_hashing::PGMHash<std::uint64_t, 2, 2, 500000000, float>;

  using MURMUR = hashing::MurmurFinalizer<Key>;
  using MultPrime64 = hashing::MultPrime64;
  using FibonacciPrime64 = hashing::FibonacciPrime64;
  using AquaHash = hashing::AquaHash<Key>;
  using XXHash3 = hashing::XXHash3<Key>;
  using MWHC = exotic_hashing::MWHC<Key>;
  using BitMWHC = exotic_hashing::BitMWHC<Key>;
  using RecSplit = exotic_hashing::RecSplit<std::uint64_t>;

  // 8
  // CollisionBM(RMIHash_10, 10);
  // CollisionBM(RMIHash_100, 100);
  // CollisionBM(RMIHash_1k, 1000);
  // CollisionBM(RMIHash_10k, 10000);
  // CollisionBM(RMIHash_100k, 100000);
  // CollisionBM(RMIHash_1M, 1000000);
  // CollisionBM(RMIHash_10M, 10000000);
  // CollisionBM(RMIHash_100M, 100000000);

  // // 5
  // CollisionBM(RadixSplineHash_4, 4);
  // CollisionBM(RadixSplineHash_16, 16);
  // CollisionBM(RadixSplineHash_128, 128);
  // CollisionBM(RadixSplineHash_1k, 1024);
  CollisionBM(RadixSplineHash_100k, 100000);

  // 5
  CollisionBM(PGMHash_2, 2);
  CollisionBM(PGMHash_100, 100);
  CollisionBM(PGMHash_1k, 1000);
  CollisionBM(PGMHash_32, 32);
  CollisionBM(PGMHash_100k, 100000);

  // 8
  CollisionBM(MURMUR,0);
  CollisionBM(MultPrime64,0);
  CollisionBM(FibonacciPrime64,0);
  CollisionBM(AquaHash,0);
  CollisionBM(XXHash3,0);
  CollisionBM(MWHC,0);
  CollisionBM(BitMWHC,0);
  CollisionBM(RecSplit,0);

} // namespace _
