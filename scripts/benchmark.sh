#!/bin/bash
set -e          # Stop in case there is an error

INITIAL_DIR=$(pwd)
_source_dir_=$(dirname "$0")
BASE_DIR=$(readlink -f "${_source_dir_}/..")     # /home/ilaria/Documents/stage/hashing-benchmark-docker

# Default values
thread_number=$(nproc --all)
filter=""

# Function to display usage information
function show_usage {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -t, --threads THREADS     Number of threads to use (default: all)"
    echo "  -f, --filter FILTER       Type of benchmarks to execute (default: all)"
    echo "                            Available options: collisions (more to be added)"
    echo "  -h, --help                Display this help message"
    exit 1
}

# Parse command line options
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--threads)
            thread_number="$2"
            shift 2
            ;;
        -f|--filter)
            filter="$2"
            shift 2
            ;;
        -h|--help)
            show_usage
            ;;
        *)
            echo "Invalid option: $1"
            show_usage
            ;;
    esac
done

# go in the code directory
cd $BASE_DIR/code
source .env

# Build the target
./build.sh benchmarks RELEASE

# Get benchmarks
if [[ "$filter" != "" ]]; then
    readarray -t all_bm < <(cmake-build-release/src/benchmarks --benchmark_list_tests | grep -i "$filter" | sed 's,/.*,*,' | uniq )
    filter="${filter}_"
else
    readarray -t all_bm < <(cmake-build-release/src/benchmarks --benchmark_list_tests | sed 's,/.*,*,' | uniq )
fi
# declare -p all_bm     # Check if the declaration went well

echo -e "\n\033[1;96m [benchmark.sh] \033[0mRunning on $thread_number threads.\n"

# Calculate the number of elements per thread
elements_per_thread=$(((${#all_bm[@]} + thread_number - 1) / thread_number))

# Working filter example
# bin/template --benchmark_filter=BM_VectorPushBack\<int,\ int\>\|BM_VectorPushBack\<int,\ double\>

set +e        # Disable "exit on error"
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

set -e          # Stop in case there is an error

echo -e "\n\033[1;96m [benchmark.sh] \033[0mBenchmark execution done!\n"

# join tmp results

# Create output file
date_string=$(date +'%Y-%m-%d-%H-%M')
output_file="../output/${date_string}_${filter}results.json"
touch $output_file

# Compute introduction length
count=$(awk '/"benchmarks"/ {print; exit} 1' ../output/tmp_results_0.json | wc -l)
count=$((count + 1))

# Copy first file
head -n -2 ../output/tmp_results_0.json >> $output_file

for ((i = 1; i < thread_number; i++)); do
    echo , >> $output_file
    tail -n +$count "../output/tmp_results_$i.json" | head -n -2 >> $output_file
done

echo -e '\t]\n}' >> $output_file

echo -e "\n\033[1;96m [benchmark.sh] \033[0mResult file is ready.\n"

# Remove temporary files
# rm ../output/tmp*.json