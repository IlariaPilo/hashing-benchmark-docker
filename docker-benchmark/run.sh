#!/bin/bash

# Thanks @ ChatGPT for the skeleton of this bash program :D

fb="fb_200M_uint64"
osm="osm_cellids_200M_uint64"
wiki="wiki_ts_200M_uint64"


# Default values for input and output directories
input_dir="../data"
output_dir="../output"

# Parse command-line options
while getopts "hi:o:" opt; do
  case $opt in
    i)
      input_dir=$OPTARG
      ;;
    o)
      output_dir=$OPTARG
      ;;
    h)
      echo "Usage: $0 [-i input_directory] [-o output_directory] [-h]"
      echo "  -i input_directory: the directory that contains the datasets"
      echo "     [default: /hashing-benchmark-docker/data]"
      echo "  -o output_directory: the directory that will store the benchmark results"
      echo "     [default: /hashing-benchmark-docker/output]"
      echo "  -h: Print this help message"
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

# Get realpaths
input_dir=$(realpath $input_dir)
output_dir=$(realpath $output_dir)

# Check if input_dir stores all datasets
if [[ $(ls $input_dir/{$fb,$osm,$wiki} 2>/dev/null | wc -l) -ne 3 ]]; then
    echo "It looks like the provided directory does not contain the required datasets.\n \
        Do you want to download them now? [y/n]"
    read answer

    if [[ $answer == "y" || $answer == "Y" ]]; then
        bash setup_datasets.sh $input_dir
    else
        echo "Operation aborted"
        exit 1
    fi
fi

docker run -v $output_dir:/home/benchmarker/hashing-benchmark-docker/output \
    -v $input_dir:/home/benchmarker/hashing-benchmark-docker/data \
    -it docker-benchmark