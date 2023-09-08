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


// const std::vector<std::int64_t> dataset_sizes{100000000};
// const std::vector<std::int64_t> datasets{
//     static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::UNIFORM)
//     // static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::GAPPED_10),
//     // static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::SEQUENTIAL)
//     };
const std::vector<std::int64_t> probe_distributions{
    // static_cast<std::underlying_type_t<dataset::ProbingDistribution>>(
    //     dataset::ProbingDistribution::EXPONENTIAL_SORTED),
    // static_cast<std::underlying_type_t<dataset::ProbingDistribution>>(
    //     dataset::ProbingDistribution::EXPONENTIAL_RANDOM),
    static_cast<std::underlying_type_t<dataset::ProbingDistribution>>(
        dataset::ProbingDistribution::UNIFORM)};

const std::vector<std::int64_t> dataset_sizes{100000000};
const std::vector<std::int64_t> succ_probability{100};
const std::vector<std::int64_t> point_query_prop{0,10,20,30,40,50,60,70,80,90,100};
const std::vector<std::int64_t> datasets{
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::WIKI),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::GAPPED_10),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::UNIFORM),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::NORMAL),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::SEQUENTIAL),
    // static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::OSM),
    static_cast<std::underlying_type_t<dataset::ID>>(dataset::ID::FB)
    };

// *********************************************************************** //

// IMPORTANT - this is a benchmarked function
// builds the hash table
template <class Table>
static void Construction(benchmark::State& state) {
  std::random_device rd;
  std::default_random_engine rng(rd());

  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));  // which dataset?

  // Generate data (keys & payloads) & probing set
  std::vector<std::pair<Key, Payload>> data;
  data.reserve(dataset_size);
  {
    auto keys = dataset::load_cached<Key>(did, dataset_size);
    std::transform(keys.begin(), keys.end(), std::back_inserter(data),
                   [](const Key& key) { return std::make_pair(key, key - 5); });
  }

  if (data.empty()) {
    // otherwise google benchmark produces an error ;(
    for (auto _ : state) {
    }
    return;
  }

  // build table
  size_t total_bytes, directory_bytes;
  std::string name;
  for (auto _ : state) {
    Table table(data);
    total_bytes = table.byte_size();
    directory_bytes = table.directory_byte_size();
    name = table.name();
  }

  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = total_bytes;
  state.counters["table_directory_bytes"] = directory_bytes;
  state.counters["table_model_bytes"] = total_bytes - directory_bytes;
  state.counters["table_bits_per_key"] = 8. * total_bytes / data.size();
  state.counters["data_elem_count"] = data.size();
  state.SetLabel(name + ":" + dataset::name(did));
}

// *********************************************************************** //

std::string previous_signature = "";
std::vector<Key> probing_set{};
void* prev_table = nullptr;
std::function<void()> free_lambda = []() {};

// *********************************************************************** //

// IMPORTANT - this is a benchmarked function
// queries (=probes) all values in the table
template <class Table, size_t RangeSize>
static void TableProbe(benchmark::State& state) {
  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));      // which datasets?
  // determines the distribution that will be used to get the probing order
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));

  // google benchmark will run a benchmark function multiple times
  // to determine, amongst other things, the iteration count for
  // the benchmark loop. Technically, BM functions must be pure. However,
  // since this setup logic is very expensive, we cache setup based on
  // a unique signature containing all parameters.
  // NOTE: google benchmark's fixtures suffer from the same
  // 'execute setup multiple times' issue:
  // https://github.com/google/benchmark/issues/952
  std::string signature =
      std::string(typeid(Table).name()) + "_" + std::to_string(RangeSize) +
      "_" + std::to_string(dataset_size) + "_" + dataset::name(did) + "_" +
      dataset::name(probing_dist);

  // if it is a new signature, it is time to initialize!
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
      // generates a probing order for any dataset dataset, given a desired distribution
      probing_set = dataset::generate_probing_set(keys, probing_dist,succ_probability);
    }
    if (data.empty()) {
      // otherwise google benchmark produces an error ;(
      for (auto _ : state) {
      }
      std::cerr << "failed" << std::endl;
      return;
    }
    // build table
    if (prev_table != nullptr) free_lambda();
    prev_table = new Table(data);
    free_lambda = []() { delete ((Table*)prev_table); };
    // measure time elapsed
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    #if PRINT
    std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
              << std::endl;
    #endif
  }

  previous_signature = signature;

  assert(prev_table != nullptr);
  Table* table = (Table*)prev_table;

  size_t i = 0;

  // start benchmarking!
  for (auto _ : state) {
    while (unlikely(i >= probing_set.size())) i -= probing_set.size();
    const auto searched = probing_set[i++];

    // Lower bound lookup
    // Returns an iterator pointing to the payload for a given key or end() if no such key could be found
    auto it = table->operator[](
        searched);  // TODO: does this generate a 'call' op? =>
                    // https://stackoverflow.com/questions/10631283/how-will-i-know-whether-inline-function-is-actually-replaced-at-the-place-where

    // RangeSize == 0 -> only key lookup
    // RangeSize == 1 -> key + payload lookup
    // RangeSize  > 1 -> lb key + multiple payload lookups
    if constexpr (RangeSize == 0) {
      benchmark::DoNotOptimize(it);
    } else if constexpr (RangeSize == 1) {
      const auto payload = it.payload();
      benchmark::DoNotOptimize(payload);
    } else if constexpr (RangeSize > 1) {
      Payload total = 0;
      for (size_t i = 0; it != table->end() && i < RangeSize; i++, ++it) {
        total ^= it.payload();
      }
      benchmark::DoNotOptimize(total);
    }
    full_mem_barrier;
  }

  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = table->byte_size();
  state.counters["table_directory_bytes"] = table->directory_byte_size();
  state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;
  state.SetLabel(table->name() + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist));
}

// *********************************************************************** //

// IMPORTANT - this is a benchmarked function
// queries (=probes) all values in the table. 
// each query will be ranged (with range 10) with probability 1-percentage_of_point_queries/100
template <class Table>
static void TableMixedLookup(benchmark::State& state) {
  std::random_device rd;
  std::default_random_engine rng(rd());

  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));
  const auto percentage_of_point_queries = static_cast<size_t>(state.range(3));

  // google benchmark will run a benchmark function multiple times
  // to determine, amongst other things, the iteration count for
  // the benchmark loop. Technically, BM functions must be pure. However,
  // since this setup logic is very expensive, we cache setup based on
  // a unique signature containing all parameters.
  // NOTE: google benchmark's fixtures suffer from the same
  // 'execute setup multiple times' issue:
  // https://github.com/google/benchmark/issues/952
  std::string signature = std::string(typeid(Table).name()) + "_" +
                          std::to_string(percentage_of_point_queries) + "_" +
                          std::to_string(dataset_size) + "_" +
                          dataset::name(did) + "_" +
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
      return;
    }
    // build table
    if (prev_table != nullptr) free_lambda();
    prev_table = new Table(data);
    free_lambda = []() { delete ((Table*)prev_table); };
    // measure time elapsed
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    #if PRINT
    std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
              << std::endl;
    #endif
  }
  previous_signature = signature;

  assert(prev_table != nullptr);
  Table* table = (Table*)prev_table;

  // distribution
  // we use this to generate a random value between 1 and 100
  std::uniform_int_distribution<size_t> point_query_dist(1, 100);

  size_t i = 0;
  for (auto _ : state) {
    while (unlikely(i >= probing_set.size())) i -= probing_set.size();
    const auto searched = probing_set[i++];

    // Lower bound lookup
    auto it = table->operator[](searched);
    if (it != table->end()) {
      const auto lb_payload = it.payload();
      benchmark::DoNotOptimize(lb_payload);

      // Chance based perform full range scan
      // performs range query with probability 1-percentage_of_point_queries/100
      if (point_query_dist(rng) > percentage_of_point_queries) {
        ++it;
        Payload total = 0;
        for (size_t i = 1; it != table->end() && i < 10; i++, ++it) {
          total ^= it.payload();
        }
        benchmark::DoNotOptimize(total);
      }
    }

    full_mem_barrier;
  }

  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = table->byte_size();
  state.counters["point_lookup_percent"] =
      static_cast<double>(percentage_of_point_queries) / 100.0;
  state.counters["table_directory_bytes"] = table->directory_byte_size();
  state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;
  state.SetLabel(table->name() + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist));
}


// *********************************************************************** //

// IMPORTANT - this is a benchmarked function
//
template <class Table,size_t RangeSize>
static void PointProbe(benchmark::State& state) {
  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));
  // TODO idk
  const auto succ_probability =
      static_cast<size_t>(state.range(3)); 
         

  // google benchmark will run a benchmark function multiple times
  // to determine, amongst other things, the iteration count for
  // the benchmark loop. Technically, BM functions must be pure. However,
  // since this setup logic is very expensive, we cache setup based on
  // a unique signature containing all parameters.
  // NOTE: google benchmark's fixtures suffer from the same
  // 'execute setup multiple times' issue:
  // https://github.com/google/benchmark/issues/952
  std::string signature =
      std::string(typeid(Table).name()) + "_" + std::to_string(RangeSize) +
      "_" + std::to_string(dataset_size) + "_" + dataset::name(did) + "_" +
      dataset::name(probing_dist);

  // if(previous_signature!=signature) 
  // {
  //   std::cout<<"Probing set size is: "<<probing_set.size()<<std::endl;
  //   std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
  // }
     
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
      // int succ_probability=100;
      probing_set = dataset::generate_probing_set(keys, probing_dist,succ_probability);
    }
    if (data.empty()) {
      // otherwise google benchmark produces an error ;(
      for (auto _ : state) {
      }
      std::cerr << "failed" << std::endl;
      return;
    }
    // build table
    if (prev_table != nullptr) free_lambda();
    prev_table = new Table(data);
    free_lambda = []() { delete ((Table*)prev_table); };
    // measure time elapsed
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    #if PRINT
    std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
              << std::endl;
    #endif
    // ------ this is extra wrt to TableProbe ------ //
    std::sort(data.begin(), data.end(),[](const auto& a, const auto& b) { return a.first < b.first; });
    #if PRINT
    std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
    #endif

    Table* table = (Table*)prev_table;

    #if PRINT
    table->print_data_statistics();
    #endif

    // TODO - these are useless? their scope finishes right after?
    uint64_t total_sum=0;
    uint64_t query_count=100000;
    // --------------------------------------------- //
  }
  
  assert(prev_table != nullptr);
  Table* table = (Table*)prev_table;
  previous_signature = signature;  

  size_t i = 0;
  // benchmarking time!
  for (auto _ : state) {
    while (unlikely(i >= probing_set.size())) i -= probing_set.size();
    const auto searched = probing_set[i%probing_set.size()];
    i++;

    // Lower bound lookup
    auto it = table->operator[](searched);
    benchmark::DoNotOptimize(it);

    // __sync_synchronize();
    // full_mem_barrier;
  }

  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = table->byte_size();
  state.counters["table_directory_bytes"] = table->directory_byte_size();
  state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;

  std::stringstream ss;
  ss << succ_probability;
  std::string temp = ss.str();
  state.SetLabel(table->name() + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist)+":"+temp);
}


template <class Table,size_t RangeSize>
static void CollisionStats(benchmark::State& state) {
  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));
   const auto succ_probability =
      static_cast<size_t>(state.range(3));    

  // google benchmark will run a benchmark function multiple times
  // to determine, amongst other things, the iteration count for
  // the benchmark loop. Technically, BM functions must be pure. However,
  // since this setup logic is very expensive, we cache setup based on
  // a unique signature containing all parameters.
  // NOTE: google benchmark's fixtures suffer from the same
  // 'execute setup multiple times' issue:
  // https://github.com/google/benchmark/issues/952
  std::string signature =
      std::string(typeid(Table).name()) + "_" + std::to_string(RangeSize) +
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
      // int succ_probability=100;
      probing_set = dataset::generate_probing_set(keys, probing_dist,succ_probability);
    }

    if (data.empty()) {
      // otherwise google benchmark produces an error ;(
      for (auto _ : state) {
      }
      std::cerr << "failed" << std::endl;
      return;
    }

    // build table
    if (prev_table != nullptr) free_lambda();
    prev_table = new Table(data);
    free_lambda = []() { delete ((Table*)prev_table); };

    // measure time elapsed
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    #if PRINT
    std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
              << std::endl;
    #endif
    
  }
  

  assert(prev_table != nullptr);
  Table* table = (Table*)prev_table;

  #if PRINT
  if (previous_signature != signature)
  {
    std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
    table->print_data_statistics();
  }
  #endif

  // std::cout<<"signature swap"<<std::endl;

  previous_signature = signature;  

  // std::cout<<"again?"<<std::endl;

  size_t i = 0;
  for (auto _ : state) {
    // while (unlikely(i >= probing_set.size())) i -= probing_set.size();
    // const auto searched = probing_set[i%probing_set.size()];
    // i++;

    // Lower bound lookup
    auto it = table->useless_func();  // TODO: does this generate a 'call' op? =>
                    // https://stackoverflow.com/questions/10631283/how-will-i-know-whether-inline-function-is-actually-replaced-at-the-place-where

    benchmark::DoNotOptimize(it);
    __sync_synchronize();
    // full_mem_barrier;
  }
  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = table->byte_size();
  state.counters["table_directory_bytes"] = table->directory_byte_size();
  state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;

  std::stringstream ss;
  ss << succ_probability;
  std::string temp = ss.str();
  state.SetLabel(table->name() + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist)+":"+temp);
}

using namespace masters_thesis;

// defines a macro to register all functions with a different "Table" subclass
// -- Construction
// -- TableMixedLookup
// -- TableProbe (0,1,10,20)  -> TODO: what are these numbers
#define BM(Table)                                                              \
  BENCHMARK_TEMPLATE(Construction, Table)                                      \
      ->ArgsProduct({dataset_sizes, datasets});                                \
  BENCHMARK_TEMPLATE(TableMixedLookup, Table)                                  \
      ->ArgsProduct(                                                           \
          {{100000000}, datasets, probe_distributions, {0, 25, 50, 75, 100}}); \
  BENCHMARK_TEMPLATE(TableProbe, Table, 0)                                     \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions});           \
  BENCHMARK_TEMPLATE(TableProbe, Table, 1)                                     \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions});           \
  BENCHMARK_TEMPLATE(TableProbe, Table, 10)                                    \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions});           \
  BENCHMARK_TEMPLATE(TableProbe, Table, 20)                                    \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions});

#define BenchmarkMonotone(BucketSize, Model)                    \
  using MonotoneHashtable##BucketSize##Model =                  \
      MonotoneHashtable<Key, Payload, BucketSize, Model>;       \
  BM(MonotoneHashtable##BucketSize##Model);                     \
  using PrefetchedMonotoneHashtable##BucketSize##Model =        \
      MonotoneHashtable<Key, Payload, BucketSize, Model, true>; \
  BM(PrefetchedMonotoneHashtable##BucketSize##Model);

#define BenchmarkMMPHFTable(MMPHF)                           \
  using MMPHFTable##MMPHF = MMPHFTable<Key, Payload, MMPHF>; \
  BM(MMPHFTable##MMPHF);

#define KAPILBM(Table)                                                              \
  BENCHMARK_TEMPLATE(PointProbe, Table, 0)                                     \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions,succ_probability});

// ############################## Chaining ##############################

#define BenchmarKapilChained(BucketSize,OverAlloc,HashFn)                           \
  using KapilChainedHashTable##BucketSize##OverAlloc##HashFn = KapilChainedHashTable<Key, Payload, BucketSize,OverAlloc, HashFn>; \
  KAPILBM(KapilChainedHashTable##BucketSize##OverAlloc##HashFn);

#define BenchmarKapilChainedExotic(BucketSize,OverAlloc,MMPHF)                           \
  using KapilChainedExoticHashTable##BucketSize##MMPHF = KapilChainedExoticHashTable<Key, Payload, BucketSize,OverAlloc, MMPHF>; \
  KAPILBM(KapilChainedExoticHashTable##BucketSize##MMPHF);

#define BenchmarKapilChainedModel(BucketSize,OverAlloc,Model)                           \
  using KapilChainedModelHashTable##BucketSize##OverAlloc##Model = KapilChainedModelHashTable<Key, Payload, BucketSize,OverAlloc, Model>; \
  KAPILBM(KapilChainedModelHashTable##BucketSize##OverAlloc##Model);

const std::vector<std::int64_t> bucket_size_chain{1,2,4,8};
const std::vector<std::int64_t> overalloc_chain{10,25,50,100};

// ############################## LINEAR PROBING ##############################

#define BenchmarKapilLinear(BucketSize,OverAlloc,HashFn)                           \
  using KapilLinearHashTable##BucketSize##OverAlloc##HashFn = KapilLinearHashTable<Key, Payload, BucketSize,OverAlloc, HashFn>; \
  KAPILBM(KapilLinearHashTable##BucketSize##OverAlloc##HashFn);

#define BenchmarKapilLinearExotic(BucketSize,OverAlloc,MMPHF)                           \
  using KapilLinearExoticHashTable##BucketSize##MMPHF = KapilLinearExoticHashTable<Key, Payload, BucketSize,OverAlloc, MMPHF>; \
  KAPILBM(KapilLinearExoticHashTable##BucketSize##MMPHF);

#define BenchmarKapilLinearModel(BucketSize,OverAlloc,Model)                           \
  using KapilLinearModelHashTable##BucketSize##OverAlloc##Model = KapilLinearModelHashTable<Key, Payload, BucketSize,OverAlloc, Model>; \
  KAPILBM(KapilLinearModelHashTable##BucketSize##OverAlloc##Model);

// ############################## CUCKOO HASHING ##############################

template <class Table,size_t RangeSize>
static void PointProbeCuckoo(benchmark::State& state) {
  // Extract variables
  const auto dataset_size = static_cast<size_t>(state.range(0));
  const auto did = static_cast<dataset::ID>(state.range(1));
  const auto probing_dist =
      static_cast<dataset::ProbingDistribution>(state.range(2));
  const auto succ_probability =
      static_cast<size_t>(state.range(3));
  // google benchmark will run a benchmark function multiple times
  // to determine, amongst other things, the iteration count for
  // the benchmark loop. Technically, BM functions must be pure. However,
  // since this setup logic is very expensive, we cache setup based on
  // a unique signature containing all parameters.
  // NOTE: google benchmark's fixtures suffer from the same
  // 'execute setup multiple times' issue:
  // https://github.com/google/benchmark/issues/952
  std::string signature =
      std::string(typeid(Table).name()) + "_" + std::to_string(RangeSize) +
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
      // int succ_probability=100;
      probing_set = dataset::generate_probing_set(keys, probing_dist,succ_probability);
    }

    if (data.empty()) {
      // otherwise google benchmark produces an error ;(
      for (auto _ : state) {
      }
      std::cerr << "failed" << std::endl;
      return;
    }

    // build table
    if (prev_table != nullptr) free_lambda();
    prev_table = new Table(data);
    free_lambda = []() { delete ((Table*)prev_table); };

    // measure time elapsed
    const auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;
    #if PRINT
    std::cout << "succeeded in " << std::setw(9) << diff.count() << " seconds"
              << std::endl;
    #endif
  }
  

  assert(prev_table != nullptr);
  Table* table = (Table*)prev_table;

  #if PRINT
  if (previous_signature != signature)
  {
    std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
    table->print_data_statistics();
  }
  #endif


  // if (previous_signature != signature)
  // {
  //   std::cout<<"Probing set size is: "<<probing_set.size()<<std::endl;
  //   std::cout<<std::endl<<" Dataset Size: "<<std::to_string(dataset_size) <<" Dataset: "<< dataset::name(did)<<std::endl;
  //   table->print_data_statistics();

   

  //    auto start = std::chrono::high_resolution_clock::now(); 

  //   for(int itr=0;itr<probing_set.size()*0.01;itr++)
  //   {
  //     const auto searched = probing_set[itr%probing_set.size()];
  //     // i++;
  //     // table->hash_val(searched);
  //     // Lower bound lookup
  //    table->insert(searched,searched);  // TODO: does this generate a 'call' op? =>
  //                     // https://stackoverflow.com/questions/10631283/how-will-i-know-whether-inline-function-is-actually-replaced-at-the-place-where
      
      
  //     // __sync_synchronize();
  //   }

  //    auto stop = std::chrono::high_resolution_clock::now(); 
  //   // auto duration = duration_cast<milliseconds>(stop - start); 
  //   auto duration = duration_cast<std::chrono::nanoseconds>(stop - start); 
  //   std::cout << "Insert Latency is: "<< duration.count()*100.00/probing_set.size() << " nanoseconds" << std::endl;

  
  // }


  // std::cout<<"signature swap"<<std::endl;

  previous_signature = signature;  

  // std::cout<<"again?"<<std::endl;

  
  size_t i = 0;
  for (auto _ : state) {
    while (unlikely(i >= probing_set.size())) i -= probing_set.size();
    const auto searched = probing_set[i++];

    // Lower bound lookup
    auto it = table->lookup(searched);  // TODO: does this generate a 'call' op? =>
                    // https://stackoverflow.com/questions/10631283/how-will-i-know-whether-inline-function-is-actually-replaced-at-the-place-where

    benchmark::DoNotOptimize(it);
    // __sync_synchronize();
    // full_mem_barrier;
  }

  // set counters (don't do this in inner loop to avoid tainting results)
  state.counters["table_bytes"] = table->byte_size();
  state.counters["table_directory_bytes"] = table->directory_byte_size();
  state.counters["table_bits_per_key"] = 8. * table->byte_size() / dataset_size;
  state.counters["data_elem_count"] = dataset_size;

  std::stringstream ss;
  ss << succ_probability;
  std::string temp = ss.str();
  state.SetLabel(table->name() + ":" + dataset::name(did) + ":" +
                 dataset::name(probing_dist)+":"+temp);
}

#define KAPILBMCuckoo(Table)                                                              \
  BENCHMARK_TEMPLATE(PointProbeCuckoo, Table, 0)                                     \
      ->ArgsProduct({dataset_sizes, datasets, probe_distributions,succ_probability});

#define BenchmarKapilCuckoo(BucketSize,OverAlloc,HashFn,KickingStrat)                           \
  using MURMUR1 = hashing::MurmurFinalizer<Key>; \
  using KapilCuckooHashTable##BucketSize##OverAlloc##HashFn##KickingStrat = kapilhashtable::KapilCuckooHashTable<Key, Payload, BucketSize,OverAlloc, HashFn, MURMUR1,KickingStrat>; \
  KAPILBMCuckoo(KapilCuckooHashTable##BucketSize##OverAlloc##HashFn##KickingStrat);

#define BenchmarKapilCuckooModel(BucketSize,OverAlloc,HashFn,KickingStrat1)                           \
  using MURMUR1 = hashing::MurmurFinalizer<Key>; \
  using KapilCuckooModelHashTable##BucketSize##OverAlloc##HashFn##KickingStrat1 = kapilmodelhashtable::KapilCuckooModelHashTable<Key, Payload, BucketSize,OverAlloc, HashFn, MURMUR1,KickingStrat1>; \
  KAPILBMCuckoo(KapilCuckooModelHashTable##BucketSize##OverAlloc##HashFn##KickingStrat1);

#define BenchmarKapilCuckooExotic(BucketSize,OverAlloc,HashFn,KickingStrat1)                           \
  using MURMUR1 = hashing::MurmurFinalizer<Key>; \
  using KapilCuckooModelHashTable##BucketSize##OverAlloc##HashFn##KickingStrat1 = kapilcuckooexotichashtable::KapilCuckooExoticHashTable<Key, Payload, BucketSize,OverAlloc, HashFn, MURMUR1,KickingStrat1>; \
  KAPILBMCuckoo(KapilCuckooModelHashTable##BucketSize##OverAlloc##HashFn##KickingStrat1);

// ******************* from now on, it's going to be automatic generated code ******************* //

	// Kicking strategy (for Cuckoo)
	using KickingStrat = kapilmodelhashtable::KapilModelBiasedKicking<5>;

	// Function aliases definition
	using RMIHashChained = learned_hashing::RMIHash<std::uint64_t,100>;
	using RMIHashCuckoo = learned_hashing::RMIHash<std::uint64_t,100000>;
	using RMIHashLinear = learned_hashing::RMIHash<std::uint64_t,100>;
	using RadixSplineHashChained = learned_hashing::RadixSplineHash<std::uint64_t,18,1024,100000000>;
	using RadixSplineHashCuckoo = learned_hashing::RadixSplineHash<std::uint64_t,18,32,100000000>;
	using RadixSplineHashLinear = learned_hashing::RadixSplineHash<std::uint64_t,18,1024,100000000>;
	using MURMUR = hashing::MurmurFinalizer<Key>;
	using MultPrime64 = hashing::MultPrime64;
	using FibonacciPrime64 = hashing::FibonacciPrime64;
	using AquaHash = hashing::AquaHash<Key>;
	using XXHash3 = hashing::XXHash3<Key>;
	using MWHC = exotic_hashing::MWHC<Key>;
	using BitMWHC = exotic_hashing::BitMWHC<Key>;

	// Benchmarks definitions

	// --------------- RMIHash --------------- // 
	// Chained 
	BenchmarKapilChainedModel(1,10050,RMIHashChained);
	BenchmarKapilChainedModel(1,10067,RMIHashChained);
	BenchmarKapilChainedModel(1,10080,RMIHashChained);
	BenchmarKapilChainedModel(1,0,RMIHashChained);
	BenchmarKapilChainedModel(1,34,RMIHashChained);
	BenchmarKapilChainedModel(1,100,RMIHashChained);
	BenchmarKapilChainedModel(1,300,RMIHashChained);

	// Cuckoo 
	BenchmarKapilCuckooModel(4,34,RMIHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,25,RMIHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,17,RMIHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,11,RMIHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,5,RMIHashCuckoo,KickingStrat);

	// Linear 
	BenchmarKapilLinearModel(1,34,RMIHashLinear);
	BenchmarKapilLinearModel(1,54,RMIHashLinear);
	BenchmarKapilLinearModel(1,82,RMIHashLinear);
	BenchmarKapilLinearModel(1,122,RMIHashLinear);
	BenchmarKapilLinearModel(1,185,RMIHashLinear);
	BenchmarKapilLinearModel(1,300,RMIHashLinear);


	// --------------- RadixSplineHash --------------- // 
	// Chained 
	BenchmarKapilChainedModel(1,10050,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,10067,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,10080,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,0,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,34,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,100,RadixSplineHashChained);
	BenchmarKapilChainedModel(1,300,RadixSplineHashChained);

	// Cuckoo 
	BenchmarKapilCuckooModel(4,34,RadixSplineHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,25,RadixSplineHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,17,RadixSplineHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,11,RadixSplineHashCuckoo,KickingStrat);
	BenchmarKapilCuckooModel(4,5,RadixSplineHashCuckoo,KickingStrat);

	// Linear 
	BenchmarKapilLinearModel(1,34,RadixSplineHashLinear);
	BenchmarKapilLinearModel(1,54,RadixSplineHashLinear);
	BenchmarKapilLinearModel(1,82,RadixSplineHashLinear);
	BenchmarKapilLinearModel(1,122,RadixSplineHashLinear);
	BenchmarKapilLinearModel(1,185,RadixSplineHashLinear);
	BenchmarKapilLinearModel(1,300,RadixSplineHashLinear);


	// --------------- MURMUR --------------- // 
	// Chained 
	BenchmarKapilChained(1,10050,MURMUR);
	BenchmarKapilChained(1,10067,MURMUR);
	BenchmarKapilChained(1,10080,MURMUR);
	BenchmarKapilChained(1,0,MURMUR);
	BenchmarKapilChained(1,34,MURMUR);
	BenchmarKapilChained(1,100,MURMUR);
	BenchmarKapilChained(1,300,MURMUR);

	// Cuckoo 
	BenchmarKapilCuckoo(4,34,MURMUR,KickingStrat);
	BenchmarKapilCuckoo(4,25,MURMUR,KickingStrat);
	BenchmarKapilCuckoo(4,17,MURMUR,KickingStrat);
	BenchmarKapilCuckoo(4,11,MURMUR,KickingStrat);
	BenchmarKapilCuckoo(4,5,MURMUR,KickingStrat);

	// Linear 
	BenchmarKapilLinear(1,34,MURMUR);
	BenchmarKapilLinear(1,54,MURMUR);
	BenchmarKapilLinear(1,82,MURMUR);
	BenchmarKapilLinear(1,122,MURMUR);
	BenchmarKapilLinear(1,185,MURMUR);
	BenchmarKapilLinear(1,300,MURMUR);


	// --------------- MultPrime64 --------------- // 
	// Chained 
	BenchmarKapilChained(1,10050,MultPrime64);
	BenchmarKapilChained(1,10067,MultPrime64);
	BenchmarKapilChained(1,10080,MultPrime64);
	BenchmarKapilChained(1,0,MultPrime64);
	BenchmarKapilChained(1,34,MultPrime64);
	BenchmarKapilChained(1,100,MultPrime64);
	BenchmarKapilChained(1,300,MultPrime64);

	// Cuckoo 
	BenchmarKapilCuckoo(4,34,MultPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,25,MultPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,17,MultPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,11,MultPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,5,MultPrime64,KickingStrat);

	// Linear 
	BenchmarKapilLinear(1,34,MultPrime64);
	BenchmarKapilLinear(1,54,MultPrime64);
	BenchmarKapilLinear(1,82,MultPrime64);
	BenchmarKapilLinear(1,122,MultPrime64);
	BenchmarKapilLinear(1,185,MultPrime64);
	BenchmarKapilLinear(1,300,MultPrime64);


	// --------------- FibonacciPrime64 --------------- // 
	// Chained 
	BenchmarKapilChained(1,10050,FibonacciPrime64);
	BenchmarKapilChained(1,10067,FibonacciPrime64);
	BenchmarKapilChained(1,10080,FibonacciPrime64);
	BenchmarKapilChained(1,0,FibonacciPrime64);
	BenchmarKapilChained(1,34,FibonacciPrime64);
	BenchmarKapilChained(1,100,FibonacciPrime64);
	BenchmarKapilChained(1,300,FibonacciPrime64);

	// Cuckoo 
	BenchmarKapilCuckoo(4,34,FibonacciPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,25,FibonacciPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,17,FibonacciPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,11,FibonacciPrime64,KickingStrat);
	BenchmarKapilCuckoo(4,5,FibonacciPrime64,KickingStrat);

	// Linear 
	BenchmarKapilLinear(1,34,FibonacciPrime64);
	BenchmarKapilLinear(1,54,FibonacciPrime64);
	BenchmarKapilLinear(1,82,FibonacciPrime64);
	BenchmarKapilLinear(1,122,FibonacciPrime64);
	BenchmarKapilLinear(1,185,FibonacciPrime64);
	BenchmarKapilLinear(1,300,FibonacciPrime64);


	// --------------- AquaHash --------------- // 
	// Chained 
	BenchmarKapilChained(1,10050,AquaHash);
	BenchmarKapilChained(1,10067,AquaHash);
	BenchmarKapilChained(1,10080,AquaHash);
	BenchmarKapilChained(1,0,AquaHash);
	BenchmarKapilChained(1,34,AquaHash);
	BenchmarKapilChained(1,100,AquaHash);
	BenchmarKapilChained(1,300,AquaHash);

	// Cuckoo 
	BenchmarKapilCuckoo(4,34,AquaHash,KickingStrat);
	BenchmarKapilCuckoo(4,25,AquaHash,KickingStrat);
	BenchmarKapilCuckoo(4,17,AquaHash,KickingStrat);
	BenchmarKapilCuckoo(4,11,AquaHash,KickingStrat);
	BenchmarKapilCuckoo(4,5,AquaHash,KickingStrat);

	// Linear 
	BenchmarKapilLinear(1,34,AquaHash);
	BenchmarKapilLinear(1,54,AquaHash);
	BenchmarKapilLinear(1,82,AquaHash);
	BenchmarKapilLinear(1,122,AquaHash);
	BenchmarKapilLinear(1,185,AquaHash);
	BenchmarKapilLinear(1,300,AquaHash);


	// --------------- XXHash3 --------------- // 
	// Chained 
	BenchmarKapilChained(1,10050,XXHash3);
	BenchmarKapilChained(1,10067,XXHash3);
	BenchmarKapilChained(1,10080,XXHash3);
	BenchmarKapilChained(1,0,XXHash3);
	BenchmarKapilChained(1,34,XXHash3);
	BenchmarKapilChained(1,100,XXHash3);
	BenchmarKapilChained(1,300,XXHash3);

	// Cuckoo 
	BenchmarKapilCuckoo(4,34,XXHash3,KickingStrat);
	BenchmarKapilCuckoo(4,25,XXHash3,KickingStrat);
	BenchmarKapilCuckoo(4,17,XXHash3,KickingStrat);
	BenchmarKapilCuckoo(4,11,XXHash3,KickingStrat);
	BenchmarKapilCuckoo(4,5,XXHash3,KickingStrat);

	// Linear 
	BenchmarKapilLinear(1,34,XXHash3);
	BenchmarKapilLinear(1,54,XXHash3);
	BenchmarKapilLinear(1,82,XXHash3);
	BenchmarKapilLinear(1,122,XXHash3);
	BenchmarKapilLinear(1,185,XXHash3);
	BenchmarKapilLinear(1,300,XXHash3);


	// --------------- MWHC --------------- // 
	// Chained 
	BenchmarKapilChainedExotic(1,10050,MWHC);
	BenchmarKapilChainedExotic(1,10067,MWHC);
	BenchmarKapilChainedExotic(1,10080,MWHC);
	BenchmarKapilChainedExotic(1,0,MWHC);
	BenchmarKapilChainedExotic(1,34,MWHC);
	BenchmarKapilChainedExotic(1,100,MWHC);
	BenchmarKapilChainedExotic(1,300,MWHC);

	// Cuckoo 
	BenchmarKapilCuckooExotic(4,34,MWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,25,MWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,17,MWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,11,MWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,5,MWHC,KickingStrat);

	// Linear 
	BenchmarKapilLinearExotic(1,34,MWHC);
	BenchmarKapilLinearExotic(1,54,MWHC);
	BenchmarKapilLinearExotic(1,82,MWHC);
	BenchmarKapilLinearExotic(1,122,MWHC);
	BenchmarKapilLinearExotic(1,185,MWHC);
	BenchmarKapilLinearExotic(1,300,MWHC);


	// --------------- BitMWHC --------------- // 
	// Chained 
	BenchmarKapilChainedExotic(1,10050,BitMWHC);
	BenchmarKapilChainedExotic(1,10067,BitMWHC);
	BenchmarKapilChainedExotic(1,10080,BitMWHC);
	BenchmarKapilChainedExotic(1,0,BitMWHC);
	BenchmarKapilChainedExotic(1,34,BitMWHC);
	BenchmarKapilChainedExotic(1,100,BitMWHC);
	BenchmarKapilChainedExotic(1,300,BitMWHC);

	// Cuckoo 
	BenchmarKapilCuckooExotic(4,34,BitMWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,25,BitMWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,17,BitMWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,11,BitMWHC,KickingStrat);
	BenchmarKapilCuckooExotic(4,5,BitMWHC,KickingStrat);

	// Linear 
	BenchmarKapilLinearExotic(1,34,BitMWHC);
	BenchmarKapilLinearExotic(1,54,BitMWHC);
	BenchmarKapilLinearExotic(1,82,BitMWHC);
	BenchmarKapilLinearExotic(1,122,BitMWHC);
	BenchmarKapilLinearExotic(1,185,BitMWHC);
	BenchmarKapilLinearExotic(1,300,BitMWHC);

}	// namespace _