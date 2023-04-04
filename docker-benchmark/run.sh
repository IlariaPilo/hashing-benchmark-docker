#!/bin/bash

# Check if the user has provided an argument
if [ $# -eq 0 ]; then
    echo "Usage: bash run.sh <output_directory>"
    exit
fi

# Get realpath
output_directory=$(realpath $1)
echo "Output files will be copied to $output_directory"

docker run -v $output_directory:/home/benchmarker/hashing-benchmark-docker/output -it docker-benchmark