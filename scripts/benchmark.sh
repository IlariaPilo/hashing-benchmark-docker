#!/bin/bash

# Save the original working directory
original_dir=$(pwd)

function create_copy_and_execute {
    element=$1

    # Create a fully recursive copy of a directory
    new_dir="../parallel/${element}_code"
    if [ ! -d "$new_dir" ]; then
        cp -R ../code "$new_dir"
    fi

    # Move into the copied directory
    cd "$new_dir"

    # Call a Python script
    python3 benchmark_basic.py "$element"

    cd "$original_dir"
}

echo -e "\n------------------ PARALLEL HASH BENCHMARKS ------------------\n"

# List of elements to process in parallel
# targets=("learned_linear" "traditional_linear" "perfect_linear" 
#         "learned_chained" "traditional_chained" "perfect_chained"
#         "learned_cuckoo" "traditional_cuckoo" "perfect_cuckoo")
targets=("learned_linear" "traditional_linear" "perfect_linear" 
         "learned_chained" "traditional_chained" "perfect_chained"
         "learned_cuckoo" "traditional_cuckoo")

# Number of parallel processes to use
if [ -z "$1" ] || [ "$1" -gt 9 ]; then
    num_processes=9
else
    num_processes="$1"
fi

echo "starting computation with $num_processes threads"

if [ ! -d "../parallel" ]; then
    # Create the directory
    mkdir "../parallel"
fi

# Loop through the list of elements and execute them in parallel
for element in "${targets[@]}"; do
    create_copy_and_execute "$element" &
    if (( ++count == num_processes )); then
        count=0
        wait
    fi
done
wait
