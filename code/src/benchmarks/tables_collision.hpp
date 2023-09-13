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
        dataset::ProbingDistribution::UNIFORM)
};

const std::vector<std::int64_t> dataset_sizes{100000000};
// const std::vector<std::int64_t> succ_probability{100};
// const std::vector<std::int64_t> point_query_prop{0,10,20,30,40,50,60,70,80,90,100};
const std::vector<std::int64_t> datasets{
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::WIKI),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::GAPPED_10),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::UNIFORM),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::NORMAL),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::SEQUENTIAL),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::OSM),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::FB)
};

std::string previous_signature = "";
std::vector<Key> probing_set{};

void* prev_fn = nullptr;
std::function<void()> free_fn = []() {};
std::chrono::duration<double> tot_time(0);
size_t collisions_count = 0;


template <class HashFn, size_t RangeSize>
static void CollisionStats(benchmark::State& state) {
  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));
  //const auto succ_probability = static_cast<size_t>(state.range(3));    

  std::string signature =
      std::string(typeid(HashFn).name()) + "_" + std::to_string(RangeSize) +
      "_" + std::to_string(dataset_size) + "_" + dataset::name(did) + "_" +
      dataset::name(probing_dist);
  if (previous_signature != signature) {
    #if PRINT
    std::cout << "performing setup... ";
    #endif
    auto start = std::chrono::steady_clock::now();
    // Generate data (keys & payloads) & probing set
    std::vector<std::pair<Key, Payload>> data{};
    data.reserve(dataset_size);
    {
      auto keys = dataset::load_cached<Key>(did, dataset_size);
      std::transform(
          keys.begin(), keys.end(), std::back_inserter(data),
          [](const Key& key) { return std::make_pair(key, key - 5); });
      int succ_probability=100;
      probing_set = dataset::generate_probing_set(keys, probing_dist,succ_probability);
    }
    if (data.empty()) {
      // otherwise google benchmark produces an error ;(
      for (auto _ : state) {
      }
      std::cerr << "failed" << std::endl;
      return;
    }
    // at this point, we have ------ std::vector<std::pair<Key, Payload>> data
    // in case we are in a model fn
    // ensure data is sorted - we trust it, too slow :3
    // std::sort(data.begin(), data.end(),
    //          [](const auto& a, const auto& b) { return a.first < b.first; });

    // build function
    if (prev_fn != nullptr) free_fn();
    prev_fn = new HashFn();
    free_fn = []() { delete ((HashFn*)prev_fn); };

    HashFn fn = *(static_cast<HashFn*>(prev_fn));

    if constexpr (has_train_member<decltype(fn)>::value) {
      // train model on sorted data
      // obtain list of keys -> necessary for model training
      std::vector<Key> keys;
      keys.reserve(data.size());
      std::transform(data.begin(), data.end(), std::back_inserter(keys),
                     [](const auto& p) { return p.first; });
      fn.train(keys.begin(), keys.end(), dataset_size);
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
    // TODO START HERE AND DO ALL OTHER UTILITIES FUNCTIONS
    std::vector<size_t> hash_v;
    hash_v.resize(dataset_size, 0);
    HashCategories type = get_fn_type<HashFn>();

    if (type == HashCategories::UNKNOWN) {
      // maybe remove this for? it depends on the position
      for (auto _ : state) {
      }
      std::cerr << "Type is unknown. Failed :(" << std::endl;
      return;
    }
    size_t index;
    std::chrono::time_point<std::chrono::steady_clock> _start_, _end_; 

    for (auto key : probing_set) {
      switch(type) {
        case HashCategories::PERFECT:
        case HashCategories::CLASSIC:
          _start_ = std::chrono::steady_clock::now();
          index = fn(key)%dataset_size;
          _end_ = std::chrono::steady_clock::now();
          break;
        case HashCategories::LEARNED:
          _start_ = std::chrono::steady_clock::now();
          index = fn(key)/dataset_size;
          _end_ = std::chrono::steady_clock::now();
          break;
        // to remove the warning
        case HashCategories::UNKNOWN:
          break;
      }
      hash_v[index]++;
      tot_time += _end_ - _start_;
    }

    // count collisions
    for (size_t i=0; i<dataset_size; i++) {
      if (hash_v[i] != 0)
        collisions_count++;
    }
    // ********************************************************************** //
  }
  
  // assert(prev_fn != nullptr);
  // HashFn* fn = (HashFn*)prev_fn;

  #if PRINT
  if (previous_signature != signature) {
    std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
  }
  #endif

  previous_signature = signature;  

  size_t i = 0;
  for (auto _ : state) {
    // auto it = table->useless_func();
    // benchmark::DoNotOptimize(it);
    // TODO -- add logic to count collisions
    __sync_synchronize();
  }
  // // set counters (don't do this in inner loop to avoid tainting results)
  // state.counters["table_bytes"] = table->byte_size();
  // state.counters["table_directory_bytes"] = table->directory_byte_size();
  // state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;
  state.counters["tot_time_s"] = tot_time.count();
  state.counters["collisions"] = collisions_count;

  // std::stringstream ss;
  // ss << succ_probability;
  // std::string temp = ss.str();
  state.SetLabel("Collisions:" + std::string(typeid(HashFn).name()) + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist));
}

using namespace masters_thesis;

#define CollisionBM(HashFn)                                                         \
  BENCHMARK_TEMPLATE(CollisionStats, HashFn, 0)                                     \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions});

	// Function aliases definition
	// using RMIHash = learned_hashing::RMIHash<std::uint64_t,100>;

	// using RadixSplineHash = learned_hashing::RadixSplineHash<std::uint64_t,18,1024,100000000>;

	// using PGMHash = learned_hashing::PGMHash<std::uint64_t,10,10,500000000,float>;

	using MURMUR = hashing::MurmurFinalizer<Key>;
	// using MultPrime64 = hashing::MultPrime64;
	// using FibonacciPrime64 = hashing::FibonacciPrime64;
	// using AquaHash = hashing::AquaHash<Key>;
	// using XXHash3 = hashing::XXHash3<Key>;
	// using MWHC = exotic_hashing::MWHC<Key>;
	// using BitMWHC = exotic_hashing::BitMWHC<Key>;
	// using RecSplit = exotic_hashing::RecSplit<std::uint64_t>;

  CollisionBM(MURMUR);



}  // namespace _