#!/bin/bash
set -e          # Stop in case there is an error

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

cmake-build-release/src/benchmarks --benchmark_filter="${all_bm[12]}" \ 
        --benchmark_out="../output/one_result.json" --benchmark_out_format=json

