#!/bin/bash

# Save the original working directory
original_dir=$(pwd)

function create_copy_and_execute {
    element=$1

    # Create a fully recursive copy of a directory
    new_dir="../parallel/${element}_code"
    if [ ! -d "$new_dir" ]; then
        cp -R . "$new_dir"
    fi

    # Move into the copied directory
    cd "$new_dir"

    # Call a Python script
    python3 benchmark_basic.py "$element"

    cd "$original_dir"
}

echo -e "\n------------------ PARALLEL HASH BENCHMARKS ------------------\n"

# List of elements to process in parallel
targets=("learned_linear" "traditional_linear" "perfect_linear" 
         "learned_chained" "traditional_chained" "perfect_chained"
         "learned_cuckoo" "traditional_cuckoo" "perfect_cuckoo")

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

# Cleanup
# Get all output files
# Define the list of file prefixes
now=$(date +"%Y-%m-%d-%H-%M")
output_file="../output/${now}_results.json"

# Open the output file for writing
for file_prefix in "${targets[@]}"; do
    f_name="${file_prefix}_results_tmp.json"

    # Open the input file for reading
    if [ -f "$f_name" ]; then
        # Read the contents of the input file and write it to the output file
        cat "$f_name" >> "$output_file"
        # Add a newline character at the end of each file's content
        # echo >> "$output_file"

        # Remove the temporary file
        # rm "$f_name"
    fi
done
