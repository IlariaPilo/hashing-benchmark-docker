#!/bin/bash
# set -e          # Stop in case there is an error

INITIAL_DIR=$(pwd)
_source_dir_=$(dirname "$0")
BASE_DIR=$(readlink -f "${_source_dir_}/..")     # /home/ilaria/Documents/stage/hashing-benchmark-docker

# go in the code directory
cd $BASE_DIR/code
source .env

# Build the target
./build.sh benchmarks RELEASE

# Get all benchmarks
readarray -t all_bm < <(cmake-build-release/src/benchmarks --benchmark_list_tests | sed 's,/.*,*,' | uniq )
# declare -p all_bm     # Check if the declaration went well

# Get number of threads
if [ $# -eq 0 ]; then
  # Use default number
  thread_number=$(nproc --all)
else
  thread_number=$2
fi

echo -e "\n\033[1;96m [benchmark.sh] \033[0mRunning on $thread_number threads.\n"

# Calculate the number of elements per thread
elements_per_thread=$(((${#all_bm[@]} + thread_number - 1) / thread_number))

# Working filter example
# bin/template --benchmark_filter=BM_VectorPushBack\<int,\ int\>\|BM_VectorPushBack\<int,\ double\>

start=0

# Set IFS to the pipe character
IFS="|"

for ((i = 0; i < thread_number; i++)); do

    end=$((start + elements_per_thread - 1))
    # Create a slice for the current subarray
    bm_i=("${all_bm[@]:start:elements_per_thread}")

    # run benchmarks
    cmake-build-release/src/benchmarks --benchmark_filter="${bm_i[*]}" \
        --benchmark_out=../output/tmp_results_${i}.json --benchmark_out_format=json &
    
    start=$((end + 1))
done

# Reset IFS to its default value
IFS=$' \t\n'
wait

# join tmp results
# TODO
